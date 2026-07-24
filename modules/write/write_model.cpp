#include "modules/write/write_model.h"

#include "aui/dialog_service.h"
#include "aui/translation.h"
#include "base/check.h"
#include "common/format.h"
#include "common/formula_util.h"
#include "model/data_items_node_ids.h"
#include "net/net_executor_adapter.h"
#include "profile/profile.h"

namespace {
// Prepended to the control-command review for the operate stage of a
// select-before-operate command (the device has accepted the select and is
// ready to execute).
const char16_t kSecondStagePrefix[] =
    u"The remote device is ready to execute the command.\n\n";
}  // namespace

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
  }
}

std::u16string WriteModel::GetWindowTitle() const {
  return manual_ ? Translate("Manual Input") : Translate("Control");
}

std::u16string WriteModel::GetSourceTitle() const {
  return spec_.GetTitle().text;
}

std::u16string WriteModel::GetCurrentValue(bool formatted) const {
  return spec_.GetCurrentString(
      formatted ? ValueFormat{FORMAT_QUALITY | FORMAT_UNITS} : ValueFormat{0});
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
    scada::WriteFlags flags;
    flags.set_select();
    CoSpawn(executor_,
            [executor = executor_, model = weak_from_this(),
             operation = spec_.scada_node().write(
                 scada::AttributeId::Value, write_value_, flags)]() mutable {
              return CompleteWriteAsync(std::move(executor), std::move(model),
                                        std::move(operation));
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
  if (!status) {
    writing_ = true;
    auto title = GetWindowTitle();
    std::u16string message = ToString16(status) + u'.';
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
    StartWriting(true);
    return;
  }

  writing_ = true;
  completion_handler(true);
}

std::u16string WriteModel::GetConfirmationMessage(bool second_stage) const {
  // Present the operator what the point reads now and what the command will
  // make it, so an irreversible field action is reviewed — not just answered
  // yes/no — before it is sent (principle §7 in client/docs/ux/principles.md).
  const std::u16string present_str =
      spec_.GetCurrentString(ValueFormat{FORMAT_UNITS});
  const std::u16string command_str =
      spec_.GetValueString(write_value_, {}, ValueFormat{FORMAT_UNITS});

  std::u16string message;
  if (second_stage)
    message += kSecondStagePrefix;
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
  CoSpawn(executor_,
          [executor = executor_, model = weak_from_this(),
           dialog_service = dialog_service_, title = spec_.GetTitle().text,
           message = GetConfirmationMessage(second_stage)]() mutable {
            return ConfirmAndStartWritingAsync(
                std::move(executor), std::move(model), *dialog_service,
                std::move(message), std::move(title));
          });
}

void WriteModel::StartWritingHelper() {
  write_selecting_ = false;
  status_change_handler();

  // Execute actual write.
  CoSpawn(executor_, [executor = executor_, model = weak_from_this(),
                      operation = spec_.scada_node().write(
                          scada::AttributeId::Value, write_value_)]() mutable {
    return CompleteWriteAsync(std::move(executor), std::move(model),
                              std::move(operation));
  });
}

Awaitable<void> WriteModel::CompleteWriteAsync(
    AnyExecutor executor,
    std::weak_ptr<WriteModel> model,
    Awaitable<scada::Status> operation) {
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
        model_ptr->writing_ = false;
        model_ptr->completion_handler(false);
      }
    }
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
    completion_handler(true);
  } catch (...) {
  }
  co_return;
}
