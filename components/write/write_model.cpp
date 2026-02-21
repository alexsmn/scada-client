#include "components/write/write_model.h"

#include "aui/dialog_service.h"
#include "base/promise_executor.h"
#include "base/u16format.h"
#include "common/format.h"
#include "common/formula_util.h"
#include "model/data_items_node_ids.h"
#include "profile/profile.h"
#include "scada/status_promise.h"

namespace {
const wchar_t kDiscreteConfirmationQuestion[] =
    L"Перевести {} в состояние {}?";
const wchar_t kAnalogConfirmationQuestion[] = L"Записать в {} значение {}?";
const char16_t kSecondStagePrefix[] =
    u"Удаленное устройство готово к исполнению команды.\n\n";
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
  return manual_ ? u"Ручной ввод" : u"Управление";
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
    scada::BindStatusCallback(
        spec_.scada_node().call(data_items::id::DataItemType_WriteManual,
                                write_value_, lock),
        BindExecutor(
            executor_, weak_from_this(),
            [this](const scada::Status& status) { OnWriteComplete(status); }));

  } else if (two_staged_) {
    write_selecting_ = true;
    scada::WriteFlags flags;
    flags.set_select();
    scada::BindStatusCallback(
        spec_.scada_node().write(scada::AttributeId::Value, write_value_,
                                 flags),
        BindExecutor(
            executor_, weak_from_this(),
            [this](const scada::Status& status) { OnWriteComplete(status); }));

  } else {
    StartWriting(false);
  }
}

std::u16string WriteModel::GetStatusText() const {
  if (!writing_)
    return {};

  return write_selecting_ ? u"Подготовка к управлению..." : u"Управление...";
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
    dialog_service_->RunMessageBox(message, title, MessageBoxMode::Error)
        .then([completion_handler = completion_handler](
                  MessageBoxResult result) { completion_handler(true); });
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
  dialog_service_
      ->RunMessageBox(message, title, MessageBoxMode::QuestionYesNoDefaultNo)
      .then(BindPromiseExecutor(executor_, weak_from_this(),
                                [this](MessageBoxResult message_box_result) {
                                  if (message_box_result ==
                                      MessageBoxResult::Yes) {
                                    StartWritingHelper();
                                  } else {
                                    writing_ = false;
                                    completion_handler(false);
                                    return;
                                  }
                                }));
}

void WriteModel::StartWritingHelper() {
  write_selecting_ = false;
  status_change_handler();

  // Execute actual write.
  scada::BindStatusCallback(
      spec_.scada_node().write(scada::AttributeId::Value, write_value_),
      BindCancelation(weak_from_this(), [this](const scada::Status& status) {
        OnWriteComplete(status);
      }));
}
