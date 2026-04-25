#include "components/write/write_model.h"

#include "aui/dialog_service.h"
#include "aui/translation.h"
#include "base/awaitable_promise.h"
#include "base/promise_executor.h"
#include "base/u16format.h"
#include "common/format.h"
#include "common/formula_util.h"
#include "model/data_items_node_ids.h"
#include "net/net_executor_adapter.h"
#include "profile/profile.h"
#include "scada/status_exception.h"
#include "scada/status_promise.h"

namespace {
const wchar_t kDiscreteConfirmationQuestion[] =
    L"Switch {} to state {}?";
const wchar_t kAnalogConfirmationQuestion[] = L"Write value {} to {}?";
const char16_t kSecondStagePrefix[] =
    u"The remote device is ready to execute the command.\n\n";

Awaitable<scada::Status> AwaitStatus(std::shared_ptr<Executor> executor,
                                      promise<void> operation) {
  try {
    co_await AwaitPromise(NetExecutorAdapter{std::move(executor)},
                          std::move(operation));
    co_return scada::StatusCode::Good;
  } catch (...) {
    co_return scada::GetExceptionStatus(std::current_exception());
  }
}
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
  locked_ = node[data_items::id::DataItemType_Locked].value().get_or(false);
  two_staged_ =
      node[data_items::id::DataItemType_OutputTwoStaged].value().get_or(true);

  if (!manual_) {
    auto condition =
        node[data_items::id::DataItemType_OutputCondition].value().get_or(
            scada::String());
    has_condition_ = !condition.empty();
    if (has_condition_)
      condition_.Connect(timed_data_service_, condition);
  }
}

std::u16string WriteModel::GetWindowTitle() const {
  return manual_ ? Translate("Manual Input") : Translate("Control");
}

std::u16string WriteModel::GetSourceTitle() const {
  return spec_.GetTitle();
}

std::u16string WriteModel::GetCurrentValue(bool formatted) const {
  return spec_.GetCurrentString(
      formatted ? ValueFormat{FORMAT_QUALITY | FORMAT_UNITS} : ValueFormat{0});
}

std::vector<std::u16string> WriteModel::GetDiscreteStates() const {
  assert(discrete_);

  std::u16string close_label = kDefaultCloseLabel;
  std::u16string open_label = kDefaultOpenLabel;

  if (auto format = spec_.node().target(data_items::id::HasTsFormat)) {
    close_label =
        ToString16(format[data_items::id::TsFormatType_CloseLabel].value());
    open_label =
        ToString16(format[data_items::id::TsFormatType_OpenLabel].value());
  }

  return {open_label, close_label};
}

int WriteModel::GetCurrentDiscreteState() const {
  return spec_.current().value.get_or(true) ? 0 : 1;  // invert state
}

std::u16string WriteModel::GetAnalogUnits() const {
  return ToString16(
      spec_.node()[data_items::id::AnalogItemType_EngineeringUnits].value());
}

void WriteModel::Write(double value, bool lock) {
  writing_ = true;
  write_value_ = value;
  write_selecting_ = false;

  if (manual_) {
    CoSpawn(executor_, [executor = executor_, model = weak_from_this(),
                        operation = spec_.scada_node().call(
                            data_items::id::DataItemType_WriteManual,
                            write_value_, lock)]() mutable {
      return CompleteWriteAsync(std::move(executor), std::move(model),
                                std::move(operation));
    });

  } else if (two_staged_) {
    write_selecting_ = true;
    scada::WriteFlags flags;
    flags.set_select();
    CoSpawn(executor_, [executor = executor_, model = weak_from_this(),
                        operation = spec_.scada_node().write(
                            scada::AttributeId::Value, write_value_,
                            flags)]() mutable {
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

  return write_selecting_ ? Translate("Preparing to control...") : Translate("Controlling...");
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
    CoSpawn(executor_, [executor = executor_,
                        completion_handler = completion_handler,
                        dialog_service = dialog_service_, message,
                        title]() mutable {
      return ReportWriteErrorAsync(std::move(executor),
                                   std::move(completion_handler),
                                   *dialog_service, std::move(message),
                                   std::move(title));
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
  auto value_str =
      spec_.GetValueString(write_value_, {}, ValueFormat{FORMAT_UNITS});
  auto message = u16format(
      discrete_ ? kDiscreteConfirmationQuestion : kAnalogConfirmationQuestion,
      spec_.GetTitle(), value_str);
  if (second_stage)
    message.insert(0, kSecondStagePrefix);
  return message;
}

void WriteModel::StartWriting(bool second_stage) {
  if (!profile_.control_confirmation) {
    StartWritingHelper();
    return;
  }

  // Request confirmation from user.
  std::u16string title = spec_.GetTitle();
  auto message = GetConfirmationMessage(second_stage);
  CoSpawn(executor_, [executor = executor_, model = weak_from_this(),
                      prompt = dialog_service_->RunMessageBox(
                          message, title,
                          MessageBoxMode::QuestionYesNoDefaultNo)]() mutable {
    return ConfirmAndStartWritingAsync(std::move(executor), std::move(model),
                                       std::move(prompt));
  });
}

void WriteModel::StartWritingHelper() {
  write_selecting_ = false;
  status_change_handler();

  // Execute actual write.
  CoSpawn(executor_,
          [executor = executor_, model = weak_from_this(),
           operation = spec_.scada_node().write(scada::AttributeId::Value,
                                                write_value_)]() mutable {
            return CompleteWriteAsync(std::move(executor), std::move(model),
                                      std::move(operation));
          });
}

Awaitable<void> WriteModel::CompleteWriteAsync(
    std::shared_ptr<Executor> executor,
    std::weak_ptr<WriteModel> model,
    promise<void> operation) {
  auto status = co_await AwaitStatus(std::move(executor), std::move(operation));
  if (auto model_ptr = model.lock()) {
    model_ptr->OnWriteComplete(status);
  }
  co_return;
}

Awaitable<void> WriteModel::ConfirmAndStartWritingAsync(
    std::shared_ptr<Executor> executor,
    std::weak_ptr<WriteModel> model,
    promise<MessageBoxResult> prompt) {
  try {
    auto message_box_result =
        co_await AwaitPromise(NetExecutorAdapter{std::move(executor)},
                              std::move(prompt));
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
    std::shared_ptr<Executor> executor,
    std::function<void(bool ok)> completion_handler,
    DialogService& dialog_service,
    std::u16string message,
    std::u16string title) {
  try {
    co_await AwaitPromise(
        NetExecutorAdapter{std::move(executor)},
        dialog_service.RunMessageBox(message, title, MessageBoxMode::Error));
    completion_handler(true);
  } catch (...) {
  }
  co_return;
}
