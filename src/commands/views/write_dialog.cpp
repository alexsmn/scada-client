#include "commands/write_dialog.h"

#include "base/memory/weak_ptr.h"
#include "views/framework/dialog.h"
#include "timed_data/timed_data.h"
#include "timed_data/timed_data_spec.h"
#include "base/bind.h"
#include "base/threading/thread_task_runner_handle.h"
#include "core/status.h"
#include "views/client_utils_views.h"
#include "common_resources.h"
#include "services/profile.h"
#include "common/scada_node_ids.h"
#include "core/monitored_item_service.h"
#include "components/main/views/main_window_views.h"
#include "common/node_ref_util.h"
#include "common/formula_util.h"
#include "translation.h"

#include <atlbase.h>
#include <wtl/atlapp.h>
#include <wtl/atlctrls.h>

class WriteDialog : public framework::Dialog,
                    private rt::TimedDataDelegate {
 public:
  WriteDialog(TimedDataService& timed_data_service, const rt::TimedDataSpec& spec, bool manual, Profile& profile);

 protected:
  // Dialog
  virtual void OnInitDialog();
  virtual void OnOK();

 private:
  void set_running(bool running);

  void UpdateCurrent();
  void UpdateCondition();

  void OnWriteComplete(const scada::Status& status);

  // rt::TimedDataDelegate
  virtual void OnPropertyChanged(rt::TimedDataSpec& spec,
                                 const rt::PropertySet& properties);

  TimedDataService& timed_data_service_;
  Profile& profile_;

  bool				logical_;
  rt::TimedDataSpec		spec_;
  bool				manual_;
  bool				write_selecting_;
  double				write_value_;
  WTL::CComboBox		wnd_state;
  WTL::CButton		wnd_lock;

  bool has_condition_;
  rt::TimedDataSpec condition_;

  base::WeakPtrFactory<WriteDialog> weak_factory_;
};

WriteDialog::WriteDialog(TimedDataService& timed_data_service, const rt::TimedDataSpec& spec, bool manual, Profile& profile)
    : Dialog(spec.logical() ? IDD_WRITE_TS : IDD_WRITE_TIT),
      timed_data_service_(timed_data_service),
      profile_(profile),
      spec_(spec),
      logical_(spec.logical()),
      manual_(manual),
      write_selecting_(false),
      has_condition_(false),
      weak_factory_(this) {
  spec_.set_delegate(this);
  condition_.set_delegate(this);
}

void WriteDialog::OnInitDialog() {
  SetWindowText(manual_ ? L"Ручной ввод" :
                          L"Управление");
  SetItemText(IDC_TITLE, spec_.GetTitle());
  
  UpdateCurrent();

  const auto& node = spec_.GetNode();

  if (logical_) {
    wnd_state = GetItem(IDC_STATE);
    if (IsInstanceOf(node, id::DiscreteItemType)) {
      auto format = node.target(id::AnalogItemType_DisplayFormat);
      std::string close_label = kDefaultCloseLabel;
      std::string open_label = kDefaultOpenLabel;
      if (format) {
        close_label = format[id::TsFormatType_CloseLabel].value().get_or(std::string());
        open_label = format[id::TsFormatType_OpenLabel].value().get_or(std::string());
      }
      wnd_state.AddString(base::SysNativeMBToWide(open_label).c_str());
      wnd_state.AddString(base::SysNativeMBToWide(close_label).c_str());
    }
    wnd_state.SetCurSel(spec_.current().value.get_or(true) ? 0 : 1);	// invert state
  
  } else {
    if (IsInstanceOf(node, id::AnalogItemType)) {
      auto units = node[id::AnalogItemType_EngineeringUnits].value().get_or(std::string());
      SetItemText(IDC_UNITS, base::SysNativeMBToWide(units));
    }
    SetItemText(IDC_VALUE, spec_.GetCurrentString(0));
  }

  wnd_lock = GetItem(IDC_LOCK);
  wnd_lock.SetCheck(BST_CHECKED);
  wnd_lock.ShowWindow(manual_ ? SW_SHOW : SW_HIDE);

  auto& condition = node ? node[id::DataItemType_OutputCondition].value().get_or(std::string()) : std::string();
  has_condition_ = !condition.empty();
  if (has_condition_)
    condition_.Connect(timed_data_service_, condition);
  UpdateCondition();
}

void WriteDialog::OnOK() {
  if (logical_) {
    bool state = wnd_state.GetCurSel() != 0;
    write_value_ = state ? 1.0 : 0.0;
  } else {
    base::string16 value_str = GetItemText(IDC_VALUE);
    if (!Parse(value_str, write_value_)) {
      MessageBox(window_handle(),
        L"Введено неверное значение. Используйте точку в качестве разделителя "
        L"десятичных разрядов.",
        GetWindowText().c_str(), MB_OK | MB_ICONSTOP);
      return;
    }
  }

  set_running(true);
  auto weak_ptr = weak_factory_.GetWeakPtr();

  scada::WriteFlags flags;
  if (manual_) {
    bool lock = wnd_lock.GetCheck() == BST_CHECKED;

    spec_.Call(id::DataItemType_WriteManual, { write_value_, lock },
        [weak_ptr](const scada::Status& status) {
          base::ThreadTaskRunnerHandle::Get()->PostTask(FROM_HERE,
              base::Bind(&WriteDialog::OnWriteComplete, weak_ptr, status));
        });

  } else {
    write_selecting_ = true;
    flags.set_select();
    spec_.Write(write_value_, flags,
        [weak_ptr](const scada::Status& status) {
          base::ThreadTaskRunnerHandle::Get()->PostTask(FROM_HERE,
              base::Bind(&WriteDialog::OnWriteComplete, weak_ptr, status));
        });
  }
}

void WriteDialog::OnWriteComplete(const scada::Status& status) {
  SetItemText(IDC_STATUS, L"");

  if (!status) {
    set_running(false);

    const base::char16* title = write_selecting_ ? L"Подготовка к управлению" :
                                             L"Управление";
    base::string16 message = Translate(status.ToString()) + L'.';
    MessageBoxW(window_handle(), message.c_str(), title, MB_OK | MB_ICONSTOP);
    return;
  }

  if (!manual_ && write_selecting_) {
    // Request confirmation from user.
    if (profile_.control_confirmation) {
      base::string16 title = spec_.GetTitle();
      base::string16 value_str = spec_.GetValueString(write_value_, {}, FORMAT_UNITS);
      base::string16 message = base::StringPrintf(
          L"Удаленное устройство готово к исполнению команды.\n\n"
          L"Перевести %ls в состояние %ls?",
          title.c_str(), value_str.c_str());
      if (::MessageBox(window_handle(), message.c_str(), GetWindowText().c_str(), 
          MB_OKCANCEL | MB_DEFBUTTON2 | MB_ICONEXCLAMATION) != IDOK) {
        set_running(false);
        return;
      }
    }

    write_selecting_ = false;
    set_running(true);

    // Execute actual Write.
    auto weak_ptr = weak_factory_.GetWeakPtr();
    spec_.Write(write_value_, scada::WriteFlags(), [weak_ptr](const scada::Status& status) {
      if (auto ptr = weak_ptr.get())
        ptr->OnWriteComplete(status);
    });

    return;
  }

  EndDialog(true);
}

void WriteDialog::set_running(bool running) {
  ::EnableWindow(GetItem(IDOK), !running);

  const base::char16* status = L"";
  if (running) {
    status = write_selecting_ ? L"Подготовка к управлению..." :
                                L"Управление...";
  }
  SetItemText(IDC_STATUS, status);
}

void WriteDialog::UpdateCurrent() {
  SetItemText(IDC_CURRENT, spec_.GetCurrentString());
}

void WriteDialog::UpdateCondition() {
  if (has_condition_) {
    const scada::DataValue& value = condition_.current();
    bool ok = !value.qualifier.general_bad() && value.value.get_or(false);
    SetItemText(IDC_CONDITION, ok ? L"Норма" : L"Нарушение");
    EnableWindow(GetItem(IDOK), ok);
  } else {
    SetItemText(IDC_CONDITION, L"Не задано");
  }
}

void WriteDialog::OnPropertyChanged(rt::TimedDataSpec& spec,
                                    const rt::PropertySet& properties) {
  if (&spec == &spec_)
    UpdateCurrent();
  else if (&spec == &condition_)
    UpdateCondition();
}

void ExecuteWriteDialog(MainWindow* main_window, TimedDataService& timed_data_service, const NodeRef& node, bool manual, Profile& profile) {
  DCHECK(main_window);

  auto window_handle = static_cast<MainWindowViews*>(main_window)->GetWindowHandle();

  const base::char16* title = manual ? L"Ручной ввод" : L"Управление";

  if (node.node_class() != scada::NodeClass::Variable) {
    ::AtlMessageBox(window_handle,
        L"Операция не поддерживается для объектов данного типа.",
        title, MB_OK | MB_ICONEXCLAMATION);
    return;
  }

  std::string formula = MakeNodeIdFormula(node.id());
  
  rt::TimedDataSpec spec;
  spec.Connect(timed_data_service, formula);

  WriteDialog dialog(timed_data_service, spec, manual, profile);
  dialog.Execute(window_handle);
}
