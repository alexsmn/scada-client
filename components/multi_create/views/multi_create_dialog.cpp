#include "components/multi_create/multi_create_dialog.h"

#include "base/strings/sys_string_conversions.h"
#include "common_resources.h"
#include "components/multi_create/multi_create_model.h"
#include "services/dialog_service.h"
#include "views/framework/control/button.h"
#include "views/framework/control/editbox.h"
#include "views/framework/dialog.h"

#include <atlbase.h>

#include <atlapp.h>
#include <atlctrls.h>

class MultiCreateDialog final : public framework::Dialog,
                                private framework::ButtonController,
                                private framework::EditBoxController {
 public:
  explicit MultiCreateDialog(MultiCreateModel& model);

 protected:
  virtual void OnInitDialog() override;
  virtual void OnOK() override;

 private:
  void SetAutoName();

  // framework::ButtonController
  virtual void OnButtonPressed(framework::Button& sender) override;

  // framework::EditBox
  virtual void OnEditBoxChanged(framework::EditBox& sender) override;

  MultiCreateModel& model_;

  framework::EditBox name_edit_;
  framework::Button ts_check_;
  framework::Button tit_check_;

  WTL::CComboBox device_combo_box_;

  bool ts_ = true;
  bool auto_name_ = true;
};

MultiCreateDialog::MultiCreateDialog(MultiCreateModel& model)
    : Dialog{IDD_NEW_ITEMS}, model_{model} {}

void MultiCreateDialog::OnInitDialog() {
  __super::OnInitDialog();

  ts_check_.Attach(GetItem(IDC_TYPE));
  AttachView(ts_check_, IDC_TYPE);
  ts_check_.SetController(this);

  tit_check_.Attach(GetItem(IDC_TYPE_TIT));
  AttachView(tit_check_, IDC_TYPE_TIT);
  tit_check_.SetController(this);

  name_edit_.Attach(GetItem(IDC_NAME_PREFIX));
  AttachView(name_edit_, IDC_NAME_PREFIX);
  name_edit_.SetController(this);

  device_combo_box_ = GetDlgItem(window_handle(), IDC_DEVICES_COMBO);

  for (auto& p : model_.devices())
    device_combo_box_.AddString(p.first.c_str());
  device_combo_box_.SetCurSel(0);

  WTL::CButton(GetDlgItem(window_handle(), IDC_TYPE)).SetCheck(BST_CHECKED);

  WTL::CUpDownCtrl(GetDlgItem(window_handle(), IDC_COUNT_SPIN))
      .SetRange(0, 1000);
  WTL::CUpDownCtrl(GetDlgItem(window_handle(), IDC_STARTING_ADDRESS_SPIN))
      .SetRange(0, 10000);
  WTL::CUpDownCtrl(GetDlgItem(window_handle(), IDC_STARTING_NUMBER_SPIN))
      .SetRange(0, 10000);

  SetAutoName();
  SetItemInt(IDC_STARTING_NUMBER, 1);
  SetItemInt(IDC_STARTING_ADDRESS, 1);
  SetItemInt(IDC_COUNT, 1);
}

void MultiCreateDialog::OnOK() {
  MultiCreateModel::RunParams params{};
  params.device = win_util::GetWindowText(device_combo_box_);
  params.name_prefix = GetItemText(IDC_NAME_PREFIX);
  params.path_prefix = base::SysWideToNativeMB(GetItemText(IDC_PATH_PREFIX));
  params.starting_number = GetItemInt(IDC_STARTING_NUMBER);
  params.starting_address = GetItemInt(IDC_STARTING_ADDRESS);
  params.count = GetItemInt(IDC_COUNT);
  params.ts = ts_;

  model_.Run(params);

  __super::OnOK();
}

void MultiCreateDialog::SetAutoName() {
  name_edit_.SetText(model_.GetAutoName(ts_));
}

void MultiCreateDialog::OnButtonPressed(framework::Button& sender) {
  ts_ = ts_check_.IsChecked();
  if (auto_name_)
    SetAutoName();
}

void MultiCreateDialog::OnEditBoxChanged(framework::EditBox& sender) {
  auto_name_ = false;
}

void ShowMultiCreateDialog(DialogService& dialog_service,
                           MultiCreateContext&& context) {
  MultiCreateModel model{std::move(context)};
  MultiCreateDialog dialog{model};
  if (dialog.Execute(dialog_service.GetDialogOwningWindow()) != IDOK)
    return;
}
