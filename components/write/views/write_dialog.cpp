#include "components/write/write_dialog.h"

#include "base/format.h"
#include "base/string_piece_util.h"
#include "base/strings/string_util.h"
#include "common_resources.h"
#include "components/write/write_model.h"
#include "services/dialog_service.h"
#include "views/client_utils_views.h"
#include "views/dialog_service_impl_views.h"
#include "views/framework/dialog.h"

#include <atlbase.h>

#include <atlapp.h>
#include <atlctrls.h>

class WriteDialog : public framework::Dialog {
 public:
  explicit WriteDialog(WriteModel& model);

 protected:
  // Dialog
  virtual void OnInitDialog() override;
  virtual void OnOK() override;

 private:
  void set_running(bool running);

  void UpdateCurrent();
  void UpdateCondition();

  DialogServiceImplViews dialog_service_;
  WriteModel& model_;

  WTL::CComboBox wnd_state;
  WTL::CButton wnd_lock;
};

WriteDialog::WriteDialog(WriteModel& model)
    : Dialog{model.discrete() ? IDD_WRITE_TS : IDD_WRITE_TIT}, model_{model} {
  model_.current_change_handler = [this] { UpdateCurrent(); };
  model_.condition_change_handler = [this] { UpdateCondition(); };

  model_.status_change_handler = [this] {
    SetItemText(IDC_STATUS, base::AsWString(model_.GetStatusText()));
  };

  model_.completion_handler = [this](bool ok) {
    set_running(false);
    if (ok)
      EndDialog(true);
  };
}

void WriteDialog::OnInitDialog() {
  dialog_service_.dialog_owning_window = window_handle();
  model_.set_dialog_service(&dialog_service_);

  SetWindowText(base::AsWString(model_.GetWindowTitle()));
  SetItemText(IDC_TITLE, base::AsWString(model_.GetSourceTitle()));

  UpdateCurrent();

  if (model_.discrete()) {
    wnd_state = GetItem(IDC_STATE);
    for (const auto& choice : model_.GetDiscreteStates())
      wnd_state.AddString(base::AsWString(choice).c_str());
    wnd_state.SetCurSel(model_.GetCurrentDiscreteState());

  } else {
    SetItemText(IDC_UNITS, base::AsWString(model_.GetAnalogUnits()));
    SetItemText(IDC_VALUE, base::AsWString(model_.GetCurrentValue(false)));
  }

  wnd_lock = GetItem(IDC_LOCK);
  wnd_lock.SetCheck(model_.locked() ? BST_CHECKED : BST_UNCHECKED);
  wnd_lock.ShowWindow(model_.lock_allowed() ? SW_SHOW : SW_HIDE);

  UpdateCondition();
}

void WriteDialog::OnOK() {
  double value = 0.0;
  if (model_.discrete()) {
    bool state = wnd_state.GetCurSel() != 0;
    value = state ? 1.0 : 0.0;

  } else {
    std::wstring value_str = GetItemText(IDC_VALUE);
    if (!Parse(AsStringView(base::AsStringPiece16(value_str)), value)) {
      dialog_service_.RunMessageBox(
          u"Введено неверное значение. Используйте точку в качестве "
          u"разделителя десятичных разрядов.",
          model_.GetWindowTitle(), MessageBoxMode::Error);
      return;
    }
  }

  bool lock = false;
  if (model_.lock_allowed())
    lock = wnd_lock.GetCheck() == BST_CHECKED;

  set_running(true);

  model_.Write(value, lock);
}

void WriteDialog::set_running(bool running) {
  ::EnableWindow(GetItem(IDOK), !running);
  SetItemText(IDC_STATUS, base::AsWString(model_.GetStatusText()));
}

void WriteDialog::UpdateCurrent() {
  SetItemText(IDC_CURRENT, base::AsWString(model_.GetCurrentValue(true)));
}

void WriteDialog::UpdateCondition() {
  if (model_.has_condition()) {
    bool ok = model_.IsConditionOk();
    SetItemText(IDC_CONDITION, ok ? L"Норма" : L"Нарушение");
    EnableWindow(GetItem(IDOK), ok);
  } else {
    SetItemText(IDC_CONDITION, L"Не задано");
  }
}

void ExecuteWriteDialog(DialogService& dialog_service, WriteContext&& context) {
  WriteModel model{std::move(context)};
  WriteDialog dialog{model};
  dialog.Execute(dialog_service.GetDialogOwningWindow());
}
