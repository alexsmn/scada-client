#include "components/create_service_item/create_service_item_dialog.h"

#include "base/strings/string_util.h"
#include "common_resources.h"
#include "components/create_service_item/create_service_item_model.h"
#include "views/framework/control/combobox.h"
#include "views/framework/dialog.h"

class CreateServiceItemDialog final : public framework::Dialog,
                                      protected framework::ComboBoxController {
 public:
  explicit CreateServiceItemDialog(CreateServiceItemModel& model);

 protected:
  // framework::Dialog
  virtual void OnInitDialog() override;
  virtual void OnOK() override;

  // framework::ComboBoxController
  virtual void OnItemChanged(framework::ComboBox& sender,
                             int old_index,
                             int new_index) override;

 private:
  void FillDevicesList();

  void FillChannelsList();

  CreateServiceItemModel& model_;

  WTL::CListBox components_list_box_;
  framework::ComboBox devices_combo_box_;
};

CreateServiceItemDialog::CreateServiceItemDialog(CreateServiceItemModel& model)
    : Dialog{IDD_NEW_SERVICE_ITEMS}, model_{model} {}

void CreateServiceItemDialog::OnInitDialog() {
  __super::OnInitDialog();

  components_list_box_ = GetItem(IDC_CHANNELS_LIST);

  devices_combo_box_.Attach(GetItem(IDC_DEVICES_COMBO));
  AttachView(devices_combo_box_, IDC_DEVICES_COMBO);
  devices_combo_box_.SetController(this);

  FillDevicesList();
  devices_combo_box_.SetCurSel(0);
  FillChannelsList();
}

void CreateServiceItemDialog::OnOK() {
  CreateServiceItemModel::RunParams params = {};

  for (int i = 0; i < components_list_box_.GetCount(); ++i) {
    if (components_list_box_.GetSel(i))
      params.component_indexes.emplace_back(i);
  }

  model_.Run(params);

  __super::OnOK();
}

void CreateServiceItemDialog::FillDevicesList() {
  devices_combo_box_.ResetContent();
  for (auto& p : model_.devices())
    devices_combo_box_.AddString(base::AsWString(p.first).c_str());
}

void CreateServiceItemDialog::FillChannelsList() {
  components_list_box_.ResetContent();
  for (auto& p : model_.components())
    components_list_box_.AddString(base::AsWString(p.first).c_str());
}

void CreateServiceItemDialog::OnItemChanged(framework::ComboBox& sender,
                                            int old_index,
                                            int new_index) {
  if (&sender == &devices_combo_box_) {
    model_.SetDeviceIndex(new_index);
    FillChannelsList();
  }
}

void ShowCreateServiceItemDialog(DialogService& dialog_service,
                                 CreateServiceItemContext&& context) {
  CreateServiceItemModel model{std::move(context)};
  CreateServiceItemDialog dialog{model};
  if (dialog.Execute() != IDOK)
    return;
}
