#include "modules/write/write_model.h"

#include "aui/dialog_service.h"
#include "aui/translation.h"
#include "base/check.h"
#include "base/u16format.h"
#include "common/format.h"
#include "common/formula_util.h"
#include "events/local_event_util.h"
#include "events/local_events.h"
#include "model/data_items_node_ids.h"
#include "model/nested_node_ids.h"
#include "model/node_id_util.h"
#include "net/net_executor_adapter.h"
#include "node_service/node_service.h"
#include "profile/profile.h"
#include "scada/co_result.h"
#include "scada/write_flags.h"
#include "services/output_two_staged_default.h"
#include "services/server_compatibility.h"

#include <charconv>

namespace {}  // namespace

WriteModel::WriteModel(WriteContext&& context)
    : WriteContext{std::move(context)} {
  spec_.property_change_handler = [this](const PropertySet& properties) {
    if (current_change_handler)
      current_change_handler();
  };

  spec_.Connect(timed_data_service_, MakeNodeIdFormula(node_id_));
  discrete_ = spec_.logical();

  condition_.property_change_handler = [this](const PropertySet& properties) {
    if (current_change_handler)
      condition_change_handler();
  };

  const auto& node = spec_.node();
  locked_ =
      node[scada::data_items::id::DataItemType_Locked].value().get_or(false);
  two_staged_ =
      node[scada::data_items::id::DataItemType_OutputTwoStaged].value().get_or(
          true);

  if (!manual_) {
    auto condition = node[scada::data_items::id::DataItemType_OutputCondition]
                         .value()
                         .get_or(scada::String());
    has_condition_ = !condition.empty();
    if (has_condition_)
      condition_.Connect(timed_data_service_, condition);

    // Whether the device can select decides what a 2.5.7 Server's accepted
    // select means (backlog 1034); fetch it now so its type is known by then.
    // An OPC server is decided from its id and has no node to fetch.
    if (two_staged_) {
      if (const scada::NodeId device_id = CommandDeviceId();
          !device_id.is_null() && !IsOpcDeviceId(device_id)) {
        spec_.node().service()->GetNode(device_id).StartFetch();
      }
    }
  }
}

scada::NodeId WriteModel::CommandDeviceId() const {
  const NodeRef& node = spec_.node();
  if (!node || !node.service())
    return {};
  const scada::String output =
      node[scada::data_items::id::DataItemType_Output].value().get_or(
          scada::String{});
  const scada::String input =
      node[scada::data_items::id::DataItemType_Input1].value().get_or(
          scada::String{});
  return GetDataItemDeviceId(*node.service(), output, input);
}

bool WriteModel::DeviceCannotSelect() const {
  const scada::NodeId device_id = CommandDeviceId();
  if (device_id.is_null())
    return false;
  // Decided from the id alone, as TaskManagerImpl does: an OPC server is not
  // a typed node, and a node service may hold no node for it.
  if (IsOpcDeviceId(device_id))
    return true;
  return GetOutputTwoStagedDefault(spec_.node().service()->GetNode(device_id),
                                   spec_.node().type_definition().node_id()) ==
         false;
}

WriteModel::~WriteModel() {
  // Safe from a destructor because the cancel holds nothing of the model: it
  // captures the prepared call, the event sink and a title (see
  // CancelOutstandingSelect), and `spec_` is still alive in this body to build
  // the call from.
  CancelOutstandingSelect();
}

std::u16string WriteModel::GetWindowTitle() const {
  return manual_ ? Translate("Manual Input") : Translate("Control");
}

std::u16string WriteModel::GetSourceTitle() const {
  return spec_.GetTitle().text;
}

std::optional<double> ParseOperatorNumber(std::u16string_view text) {
  while (!text.empty() && (text.front() == u' ' || text.front() == u'\t'))
    text.remove_prefix(1);
  while (!text.empty() && (text.back() == u' ' || text.back() == u'\t'))
    text.remove_suffix(1);
  if (text.empty())
    return std::nullopt;

  std::string ascii;
  ascii.reserve(text.size());
  for (char16_t c : text) {
    if (c > 0x7f)
      return std::nullopt;
    ascii += c == u',' ? '.' : static_cast<char>(c);
  }
  // from_chars rejects a leading '+', which an operator may well type.
  std::string_view digits = ascii;
  if (digits.starts_with('+'))
    digits.remove_prefix(1);

  double value = 0;
  const auto [end, error] =
      std::from_chars(digits.data(), digits.data() + digits.size(), value);
  if (error != std::errc{} || end != digits.data() + digits.size())
    return std::nullopt;
  return value;
}

std::u16string WriteModel::GetCurrentValue(bool formatted) const {
  // FORMAT_STATUS adds the status letters ([С] offline, [У] stale, …): the
  // server marks those states without BAD, so «?» alone would call an offline
  // reading good (backlog 923).
  return spec_.GetCurrentString(
      formatted ? ValueFormat{FORMAT_QUALITY | FORMAT_UNITS | FORMAT_STATUS}
                : ValueFormat{0});
}

std::vector<std::u16string> WriteModel::GetDiscreteStates() const {
  scada::base::Check(discrete_);

  std::u16string close_label = DefaultCloseLabel();
  std::u16string open_label = DefaultOpenLabel();

  if (auto format = spec_.node().target(scada::data_items::id::HasTsFormat)) {
    close_label = ToString16(
        format[scada::data_items::id::TsFormatType_CloseLabel].value());
    open_label = ToString16(
        format[scada::data_items::id::TsFormatType_OpenLabel].value());
  }

  return {open_label, close_label};
}

int WriteModel::GetCurrentDiscreteState() const {
  return spec_.current().value.get_or(true) ? 0 : 1;  // invert state
}

std::u16string WriteModel::GetAnalogUnits() const {
  return ToString16(
      spec_.node()[scada::data_items::id::AnalogItemType_EngineeringUnits]
          .value());
}

void WriteModel::Write(double value, bool lock) {
  writing_ = true;
  write_value_ = value;
  write_selecting_ = false;

  if (manual_) {
    CoSpawn(executor_, [executor = executor_, model = weak_from_this(),
                        operation = spec_.scada_node().call(
                            scada::data_items::id::DataItemType_WriteManual,
                            write_value_, lock)]() mutable {
      return CompleteWriteAsync(std::move(executor), std::move(model),
                                std::move(operation));
    });

  } else if (two_staged_) {
    write_selecting_ = true;
    CoSpawn(executor_, [model = weak_from_this(),
                        select = ControlNode().call(
                            scada::data_items::id::DataItemControlType_Select,
                            write_value_),
                        cancel = ControlNode().call(
                            scada::data_items::id::DataItemControlType_Cancel),
                        local_events = local_events_,
                        source_title = GetSourceTitle()]() mutable {
      return CompleteSelectAsync(std::move(model), std::move(select),
                                 std::move(cancel), local_events,
                                 std::move(source_title));
    });

  } else {
    StartWriting(false);
  }
}

std::u16string WriteModel::GetStatusText() const {
  if (!writing_)
    return {};

  return write_selecting_ ? Translate("Preparing to control...")
                          : Translate("Controlling...");
}

bool WriteModel::IsConditionOk() const {
  if (!has_condition_)
    return true;

  const scada::DataValue& value = condition_.current();
  return !value.qualifier.general_bad() && value.value.get_or(false);
}

void WriteModel::OnWriteComplete(const scada::Status& status) {
  // A Server from before 2.6 has no Control object, so it refuses the Select
  // as an unknown node before anything reaches the device. That is not the
  // device refusing to select: send the same select the 2.5 way instead.
  if (!status && write_selecting_ && two_staged_ && !legacy_control_ &&
      IsMissingOnServer(status)) {
    SelectByLegacyWrite();
    return;
  }

  if (!status) {
    // The command is over — it failed. Leaving `writing_` set kept
    // GetStatusText() answering "Controlling..." underneath the error box; that
    // was invisible only because the dialog used to close itself on the way
    // out, which is the defect this path exists to fix.
    writing_ = false;
    // A refused select armed nothing, so there is no selection to release.
    const bool select_refused = write_selecting_;
    write_selecting_ = false;

    // Nothing to report through: complete anyway rather than stranding the
    // operator on a dialog whose Control button no longer does anything. The
    // guard also keeps the `*dialog_service` below from dereferencing null —
    // `dialog_service_` is an optional collaborator, as ReportInputError's own
    // guard already acknowledges. Same shape as LimitModel::OnWriteComplete.
    if (!dialog_service_) {
      completion_handler(false);
      return;
    }

    auto title = GetWindowTitle();
    std::u16string message = select_refused ? GetSelectRefusalMessage(status)
                                            : ToString16(status) + u'.';
    CoSpawn(executor_,
            [executor = executor_, completion_handler = completion_handler,
             dialog_service = dialog_service_, message, title]() mutable {
              return ReportWriteErrorAsync(
                  std::move(executor), std::move(completion_handler),
                  *dialog_service, std::move(message), std::move(title));
            });
    return;
  }

  if (write_selecting_) {
    // Asked again now rather than kept from when the select was sent: the
    // device fetch the constructor started has had the select's round trip to
    // complete.
    if (legacy_control_)
      device_cannot_select_ = DeviceCannotSelect();
    // The select succeeded. From here until the operate is issued or the
    // operator declines, the outstation holds a selection we are responsible
    // for releasing.
    select_outstanding_ = true;
    StartWriting(true);
    return;
  }

  writing_ = true;
  ReportSuccess();
  completion_handler(true);
}

std::u16string WriteModel::GetSelectRefusalMessage(
    const scada::Status& status) {
  // Bad_NotSupported is the Server refusing two-stage control on an object
  // whose device cannot select: IEC 60870-5 commands other than a single
  // command (C_SC_NA_1), every MODBUS and OPC object, and IEC 61850 objects
  // with direct control. Said in words, because "Operation is not supported"
  // alone does not tell the operator what to do next (backlog 983). It names
  // the project engineer rather than the administrator: two-stage control is
  // a property of the object, which the engineer configures.
  if (status.code() == scada::StatusCode::Bad_NotSupported) {
    return Translate(
        "Two-stage control is not available for this object: its device "
        "cannot select a command. Nothing was sent to the device. Ask the "
        "engineer who configures the project to turn off two-stage control "
        "for this object.");
  }
  return u16format(L"{}: {}. {}", Translate("Preparation for control failed"),
                   ToString16(status),
                   Translate("The command was not executed."));
}

void WriteModel::ReportSuccess() {
  // The "report successful control" setting promises this (backlog 849);
  // before, the dialog just closed. ReportRequestResult applies the setting.
  if (!local_events_)
    return;

  std::u16string title = u16format(
      L"{} {}: {}", GetWindowTitle(), GetSourceTitle(),
      spec_.GetValueString(write_value_, {}, ValueFormat{FORMAT_UNITS}));
  ReportRequestResult(title, scada::StatusCode::Good, *local_events_, profile_);
}

std::u16string WriteModel::GetConfirmationMessage(double value,
                                                  bool second_stage) const {
  // Present the operator what the point reads now and what the command will
  // make it, so an irreversible field action is reviewed — not just answered
  // yes/no — before it is sent (principle §7 in docs/client/ux/principles.md).
  // Formatted as in the dialog itself: this is the last screen before an
  // irreversible command, so it must not drop the «?» of an invalid reading
  // (backlog 908) or the letters of an offline or stale one (backlog 923).
  const std::u16string present_str = GetCurrentValue(/*formatted=*/true);
  const std::u16string command_str =
      spec_.GetValueString(value, {}, ValueFormat{FORMAT_UNITS});

  std::u16string message;
  // A 2.5.7 Server accepts a select on a device that cannot select without
  // contacting it (backlog 1034), so nothing is waiting: do not say it is.
  if (second_stage && !(legacy_control_ && device_cannot_select_)) {
    // The operate stage of a select-before-operate command: the device has
    // accepted the select and is waiting. This was a bare u"..." literal, so
    // it could never be translated — it must go through Translate() like the
    // rest of the review.
    message += Translate("The remote device is ready to execute the command.");
    message += u"\n\n";
  }
  message += spec_.GetTitle().text;
  message += u"\n\n";
  message += Translate("Present:");
  message += u"  ";
  message += present_str;
  message += u'\n';
  message += Translate("Command:");
  message += u"  ";
  message += command_str;
  message += u"\n\n";
  message += Translate(
      "This control command is sent to physical equipment and cannot be "
      "undone remotely. Send it?");
  return message;
}

void WriteModel::StartWriting(bool second_stage) {
  if (!profile_.control_confirmation) {
    StartWritingHelper();
    return;
  }

  // Request confirmation from the user. The message/title are handed to the
  // coroutine by value so they outlive the RunMessageBox prompt (see the
  // ConfirmAndStartWritingAsync declaration).
  CoSpawn(
      executor_,
      [executor = executor_, model = weak_from_this(),
       dialog_service = dialog_service_, title = spec_.GetTitle().text,
       message = GetConfirmationMessage(write_value_, second_stage)]() mutable {
        return ConfirmAndStartWritingAsync(
            std::move(executor), std::move(model), *dialog_service,
            std::move(message), std::move(title));
      });
}

void WriteModel::StartWritingHelper() {
  write_selecting_ = false;
  status_change_handler();

  // The operate phase of a two-staged control is a Call on the Control object,
  // so the server can tell it apart from the select. A one-stage control is an
  // ordinary value write and stays one -- and so is the operate stage on a
  // Server from before 2.6, which takes the unflagged write after a flagged
  // one as the operate.
  if (legacy_control_) {
    select_outstanding_ = false;
  } else if (two_staged_) {
    select_outstanding_ = false;
    CoSpawn(executor_, [executor = executor_, model = weak_from_this(),
                        operation = ControlNode().call(
                            scada::data_items::id::DataItemControlType_Operate,
                            write_value_)]() mutable {
      return CompleteWriteAsync(std::move(executor), std::move(model),
                                std::move(operation));
    });
    return;
  }

  CoSpawn(executor_, [executor = executor_, model = weak_from_this(),
                      operation = spec_.scada_node().write(
                          scada::AttributeId::Value, write_value_)]() mutable {
    return CompleteWriteAsync(std::move(executor), std::move(model),
                              std::move(operation));
  });
}

scada::node WriteModel::ControlNode() const {
  return spec_.node().scada_node(
      MakeNestedNodeId(spec_.node().node_id(), scada::kControlObjectName));
}

void WriteModel::SelectByLegacyWrite() {
  legacy_control_ = true;
  device_cannot_select_ = DeviceCannotSelect();
  scada::WriteFlags flags;
  flags.set_select();
  // Not CompleteWriteAsync: that drops the answer once the dialog is gone, so
  // a select the device accepted after the operator closed it set no
  // `select_outstanding_` and nobody was told the device stays selected
  // (backlog 1027).
  CoSpawn(
      executor_, [model = weak_from_this(),
                  select = spec_.scada_node().write(scada::AttributeId::Value,
                                                    write_value_, flags),
                  local_events = local_events_, source_title = GetSourceTitle(),
                  device_cannot_select = device_cannot_select_]() mutable {
        return CompleteLegacySelectAsync(std::move(model), std::move(select),
                                         local_events, std::move(source_title),
                                         device_cannot_select);
      });
}

// static
void WriteModel::ReportLegacySelectionHeld(LocalEvents* local_events,
                                           std::u16string_view source_title) {
  if (!local_events)
    return;
  local_events->ReportEvent(
      LocalEvents::SEV_ERROR,
      u16format(L"{}: {} - {}. {}", Translate("Release of selection"),
                source_title,
                Translate("A Server before 2.6 cannot release a selection"),
                Translate("The device stays selected until its own "
                          "selection timeout expires.")));
}

void WriteModel::CancelOutstandingSelect() {
  if (!select_outstanding_) {
    return;
  }
  select_outstanding_ = false;
  // A Server from before 2.6 has no Cancel to send; the 2.5 Client left the
  // selection to the device's own timeout, and so does this one. But the
  // operator is told (backlog 1013): the outstation stays selected, so another
  // select on it -- from this desk or any other -- is refused until that
  // timeout expires. Same title and severity as the 2.6 path's refused Cancel,
  // so the journal reads one way whichever Server answered. Reported
  // synchronously: nothing is awaited, so the sink cannot die under it.
  // Except where the device cannot select (MODBUS, OPC, an IEC 60870-5
  // set-point): the 2.5.7 Server accepted that select without contacting the
  // device, so nothing is held, and a critical event saying otherwise would
  // fire on every declined command (backlog 1034).
  if (legacy_control_) {
    if (!device_cannot_select_)
      ReportLegacySelectionHeld(local_events_, GetSourceTitle());
    return;
  }
  // Leaving it uncancelled would hold the outstation's selection until its own
  // sboTimeout. The operator has already moved on, so a refusal must not raise
  // an error dialog over whatever they are doing now — but it must not vanish
  // either: IEC 104 and IEC 61850 implement Cancel (backlog 946), a device can
  // refuse it, and the selection then stays armed. So a refusal is reported as
  // a local event (backlog 959).
  CoSpawn(executor_, [operation = ControlNode().call(
                          scada::data_items::id::DataItemControlType_Cancel),
                      local_events = local_events_,
                      source_title = GetSourceTitle()]() mutable {
    return ReportCancelRefusalAsync(std::move(operation), local_events,
                                    std::move(source_title));
  });
}

Awaitable<void> WriteModel::ReportCancelRefusalAsync(
    scada::CoStatus operation,
    LocalEvents* local_events,
    std::u16string source_title) {
  // Taken before the wait: the answer can come after a sign-out has destroyed
  // the event module this pointer belongs to (backlog 887 rebuilds it).
  const std::weak_ptr<const void> local_events_alive =
      local_events ? local_events->lifetime() : std::weak_ptr<const void>{};
  const scada::Status status = co_await std::move(operation);
  // Bad_NotSupported is DataItemWriteImpl::Cancel's answer for a driver with
  // no Cancel. Such a driver never sends the device a select stage, so nothing
  // is left armed to report. (Devices that cannot select -- MODBUS, OPC,
  // IEC 60870-5 commands other than C_SC_NA_1, IEC 61850 direct control --
  // now refuse the select itself, so they never get this far.)
  if (status || status.code() == scada::StatusCode::Bad_NotSupported ||
      !local_events || local_events_alive.expired())
    co_return;

  // Always reported, whatever `show_write_ok` says: that setting governs
  // success reports, and this is a failure an operator has to act on.
  local_events->ReportEvent(
      LocalEvents::SEV_ERROR,
      u16format(L"{}: {} - {}. {}", Translate("Release of selection"),
                source_title, ToString16(status),
                Translate("The device may stay selected until its own "
                          "selection timeout expires.")));
  co_return;
}

Awaitable<void> WriteModel::CompleteSelectAsync(std::weak_ptr<WriteModel> model,
                                                scada::CoStatus select,
                                                scada::CoStatus cancel,
                                                LocalEvents* local_events,
                                                std::u16string source_title) {
  const scada::Status status = co_await std::move(select);
  if (auto model_ptr = model.lock()) {
    model_ptr->OnWriteComplete(status);
    co_return;
  }
  // A refused select armed nothing.
  if (!status)
    co_return;
  co_await ReportCancelRefusalAsync(std::move(cancel), local_events,
                                    std::move(source_title));
  co_return;
}

Awaitable<void> WriteModel::CompleteLegacySelectAsync(
    std::weak_ptr<WriteModel> model,
    scada::CoStatus select,
    LocalEvents* local_events,
    std::u16string source_title,
    bool device_cannot_select) {
  // Taken before the wait, as in ReportCancelRefusalAsync: a sign-out can
  // destroy the event module while the select is in flight.
  const std::weak_ptr<const void> local_events_alive =
      local_events ? local_events->lifetime() : std::weak_ptr<const void>{};
  const scada::Status status = co_await std::move(select);
  if (auto model_ptr = model.lock()) {
    model_ptr->OnWriteComplete(status);
    co_return;
  }
  // A refused select armed nothing. An accepted one is held by the device
  // with nobody left to operate it, and a Server before 2.6 has no Cancel to
  // release it with -- so say so, as closing after the answer does. A device
  // that cannot select was never contacted, so it holds nothing (backlog
  // 1034).
  if (!status || device_cannot_select || local_events_alive.expired())
    co_return;
  ReportLegacySelectionHeld(local_events, source_title);
  co_return;
}

Awaitable<void> WriteModel::CompleteWriteAsync(AnyExecutor executor,
                                               std::weak_ptr<WriteModel> model,
                                               scada::CoStatus operation) {
  auto status = co_await std::move(operation);
  if (auto model_ptr = model.lock()) {
    model_ptr->OnWriteComplete(status);
  }
  co_return;
}

Awaitable<void> WriteModel::ConfirmAndStartWritingAsync(
    AnyExecutor executor,
    std::weak_ptr<WriteModel> model,
    DialogService& dialog_service,
    std::u16string message,
    std::u16string title) {
  try {
    auto message_box_result = co_await dialog_service.RunMessageBox(
        message, title, MessageBoxMode::QuestionYesNoDefaultNo);
    if (auto model_ptr = model.lock()) {
      if (message_box_result == MessageBoxResult::Yes) {
        model_ptr->StartWritingHelper();
      } else {
        model_ptr->CancelOutstandingSelect();
        model_ptr->writing_ = false;
        model_ptr->completion_handler(false);
      }
    }
  } catch (...) {
  }
  co_return;
}

void WriteModel::ReportInputError(std::u16string message) {
  if (!dialog_service_)
    return;

  // RunMessageBox returns a *lazy* awaitable: discarded, the coroutine body
  // never starts and no box is ever shown. The caller that used to drop it
  // (WriteDialog::accept) left the dialog refusing to close with no
  // explanation at all.
  CoSpawn(executor_,
          [dialog_service = dialog_service_, message = std::move(message),
           title = GetWindowTitle()]() mutable {
            return ShowInputErrorAsync(*dialog_service, std::move(message),
                                       std::move(title));
          });
}

Awaitable<void> WriteModel::ShowInputErrorAsync(DialogService& dialog_service,
                                                std::u16string message,
                                                std::u16string title) {
  try {
    co_await dialog_service.RunMessageBox(message, title,
                                          MessageBoxMode::Error);
  } catch (...) {
  }
  co_return;
}

Awaitable<void> WriteModel::ReportWriteErrorAsync(
    AnyExecutor executor,
    std::function<void(bool ok)> completion_handler,
    DialogService& dialog_service,
    std::u16string message,
    std::u16string title) {
  try {
    co_await dialog_service.RunMessageBox(message, title,
                                          MessageBoxMode::Error);
  } catch (...) {
  }

  // False, so the dialog stays open on the value the operator chose and the
  // refused command can be corrected or retried. Completing with true took the
  // dialog away the moment its error box was dismissed, so retrying meant
  // reopening Control and re-entering the value — and for a two-staged command,
  // confirming it again. Completing outside the try matters: a dialog service
  // that throws must still release the dialog, or a failed write leaves it
  // stuck. Same shape as LimitModel::ReportWriteErrorAsync.
  completion_handler(false);
  co_return;
}
