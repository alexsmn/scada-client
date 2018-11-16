#include "components/write/write_model.h"

#include "base/bind.h"
#include "base/threading/thread_task_runner_handle.h"
#include "common/formula_util.h"
#include "common/node_service.h"
#include "common/node_util.h"
#include "common/scada_node_ids.h"
#include "core/status.h"
#include "services/dialog_service.h"
#include "services/profile.h"

namespace {
const base::char16 kDiscreteConfirmationQuestion[] =
    L"Перевести %ls в состояние %ls?";
const base::char16 kAnalogConfirmationQuestion[] =
    L"Записать в %ls значение %ls?";
const base::char16 kSecondStagePrefix[] =
    L"Удаленное устройство готово к исполнению команды.\n\n";
}  // namespace

WriteModel::WriteModel(WriteContext&& context)
    : WriteContext{std::move(context)} {
  spec_.Connect(timed_data_service_, MakeNodeIdFormula(node_id_));
  discrete_ = spec_.logical();

  spec_.property_change_handler = [this](const rt::PropertySet& properties) {
    if (current_change_handler)
      current_change_handler();
  };

  condition_.property_change_handler =
      [this](const rt::PropertySet& properties) {
        if (current_change_handler)
          condition_change_handler();
      };

  const auto node = spec_.GetNode();
  auto condition =
      node
          ? node[id::DataItemType_OutputCondition].value().get_or(std::string())
          : std::string();
  two_staged_ = node[id::DataItemType_OutputTwoStaged].value().get_or(true);
  has_condition_ = !condition.empty();
  if (has_condition_)
    condition_.Connect(timed_data_service_, condition);
}

base::string16 WriteModel::GetWindowTitle() const {
  return manual_ ? L"Ручной ввод" : L"Управление";
}

base::string16 WriteModel::GetSourceTitle() const {
  return spec_.GetTitle();
}

base::string16 WriteModel::GetCurrentValue(bool formatted) const {
  return spec_.GetCurrentString(formatted ? FORMAT_QUALITY | FORMAT_UNITS : 0);
}

std::vector<base::string16> WriteModel::GetDiscreteStates() const {
  assert(discrete_);

  base::string16 close_label = kDefaultCloseLabel;
  base::string16 open_label = kDefaultOpenLabel;

  const auto node = spec_.GetNode();
  if (auto format = node.target(id::HasTsFormat)) {
    close_label = base::SysNativeMBToWide(
        format[id::TsFormatType_CloseLabel].value().get_or(std::string()));
    open_label = base::SysNativeMBToWide(
        format[id::TsFormatType_OpenLabel].value().get_or(std::string()));
  }

  return {open_label, close_label};
}

int WriteModel::GetCurrentDiscreteState() const {
  return spec_.current().value.get_or(true) ? 0 : 1;  // invert state
}

base::string16 WriteModel::GetAnalogUnits() const {
  auto node = spec_.GetNode();
  auto units =
      node[id::AnalogItemType_EngineeringUnits].value().get_or(std::string());
  return base::SysNativeMBToWide(units);
}

void WriteModel::Write(double value, bool lock) {
  auto weak_ptr = weak_factory_.GetWeakPtr();

  write_value_ = value;
  write_selecting_ = false;

  scada::WriteFlags flags;
  if (manual_) {
    spec_.Call(id::DataItemType_WriteManual, {write_value_, lock}, {},
               [weak_ptr](const scada::Status& status) {
                 base::ThreadTaskRunnerHandle::Get()->PostTask(
                     FROM_HERE, base::Bind(&WriteModel::OnWriteComplete,
                                           weak_ptr, status));
               });

  } else if (two_staged_) {
    write_selecting_ = true;
    flags.set_select();
    spec_.Write(
        write_value_, {}, flags, [weak_ptr](const scada::Status& status) {
          base::ThreadTaskRunnerHandle::Get()->PostTask(
              FROM_HERE,
              base::Bind(&WriteModel::OnWriteComplete, weak_ptr, status));
        });

  } else {
    StartWriting(false);
  }
}

base::string16 WriteModel::GetStatusText() const {
  return write_selecting_ ? L"Подготовка к управлению..." : L"Управление...";
}

bool WriteModel::IsConditionOk() const {
  const scada::DataValue& value = condition_.current();
  return !value.qualifier.general_bad() && value.value.get_or(false);
}

void WriteModel::OnWriteComplete(const scada::Status& status) {
  if (!status) {
    auto title = GetWindowTitle();
    base::string16 message = ToString16(status) + L'.';
    dialog_service_->RunMessageBox(message, title, MessageBoxMode::Error);
    completion_handler(true);
    return;
  }

  if (write_selecting_) {
    StartWriting(true);
    return;
  }

  completion_handler(true);
}

void WriteModel::StartWriting(bool second_stage) {
  // Request confirmation from user.
  if (profile_.control_confirmation) {
    base::string16 title = spec_.GetTitle();
    base::string16 value_str =
        spec_.GetValueString(write_value_, {}, FORMAT_UNITS);
    base::string16 message = base::StringPrintf(
        discrete_ ? kDiscreteConfirmationQuestion : kAnalogConfirmationQuestion,
        title.c_str(), value_str.c_str());
    if (second_stage)
      message.insert(0, kSecondStagePrefix);
    if (dialog_service_->RunMessageBox(
            message, title, MessageBoxMode::QuestionYesNoDefaultNo) !=
        MessageBoxResult::Yes) {
      completion_handler(false);
      return;
    }
  }

  write_selecting_ = false;
  status_change_handler();

  // Execute actual Write.
  auto weak_ptr = weak_factory_.GetWeakPtr();
  spec_.Write(write_value_, {}, {}, [weak_ptr](const scada::Status& status) {
    if (auto ptr = weak_ptr.get())
      ptr->OnWriteComplete(status);
  });
}
