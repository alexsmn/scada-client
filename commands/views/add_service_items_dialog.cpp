#include "commands/add_service_items_dialog.h"

#include "base/strings/sys_string_conversions.h"
#include "client_utils.h"
#include "common/formula_util.h"
#include "common/node_id_util.h"
#include "common/node_service.h"
#include "common/node_util.h"
#include "common/scada_node_ids.h"
#include "common_resources.h"
#include "services/task_manager.h"
#include "views/framework/control/combobox.h"
#include "views/framework/dialog.h"

#include <algorithm>

class AddServiceItemsDialog final : public framework::Dialog,
                                    protected framework::ComboBoxController {
 public:
  AddServiceItemsDialog(NodeService& node_service,
                        TaskManager& task_manager,
                        const scada::NodeId& group_id);

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
  NodeRef GetSelectedDevice() const;

  void FillChannelsList();

  NodeService& node_service_;
  TaskManager& task_manager_;
  const scada::NodeId parent_id_;

  WTL::CListBox components_list_box_;
  NamedNodes components_;

  framework::ComboBox devices_combo_box_;
  NamedNodes devices_;
};

AddServiceItemsDialog::AddServiceItemsDialog(NodeService& node_service,
                                             TaskManager& task_manager,
                                             const scada::NodeId& group_id)
    : Dialog{IDD_NEW_SERVICE_ITEMS},
      node_service_{node_service},
      task_manager_{task_manager},
      parent_id_{group_id} {}

void AddServiceItemsDialog::OnInitDialog() {
  __super::OnInitDialog();

  components_list_box_ = GetItem(IDC_CHANNELS_LIST);

  devices_combo_box_.Attach(GetItem(IDC_DEVICES_COMBO));
  AttachView(devices_combo_box_, IDC_DEVICES_COMBO);
  devices_combo_box_.SetController(this);

  FillDevicesList();
  devices_combo_box_.SetCurSel(0);
  FillChannelsList();
}

void AddServiceItemsDialog::OnOK() {
  auto device = GetSelectedDevice();
  if (!device)
    return;  // TODO: Error message.

  for (int i = 0; i < components_list_box_.GetCount(); ++i) {
    if (!components_list_box_.GetSel(i))
      continue;

    auto display_name = win_util::GetListBoxItemText(components_list_box_, i);
    const auto& component = components_[i].second;
    auto formula = MakeNodeIdFormula(
        MakeNestedNodeId(device.id(), component.browse_name().name()));
    auto type_definition_id =
        IsSubtypeOf(component.data_type(), scada::id::Boolean)
            ? id::DiscreteItemType
            : id::AnalogItemType;

    scada::NodeProperties properties;
    properties.emplace_back(kObjectInput1PropTypeId, std::move(formula));
    if (type_definition_id == id::AnalogItemType)
      properties.emplace_back(id::AnalogItemType_DisplayFormat, "0.");

    task_manager_.PostInsertTask(
        scada::NodeId{}, parent_id_, type_definition_id,
        scada::NodeAttributes{}.set_display_name(std::move(display_name)),
        std::move(properties));
  }

  __super::OnOK();
}

void AddServiceItemsDialog::FillDevicesList() {
  devices_combo_box_.ResetContent();
  devices_ = GetNamedNodes(node_service_.GetNode(id::Devices), id::DeviceType);
  SortNamedNodes(devices_);
  for (auto& p : devices_)
    devices_combo_box_.AddString(p.first.c_str());
}

void AddServiceItemsDialog::FillChannelsList() {
  components_list_box_.ResetContent();
  components_.clear();

  auto node = GetSelectedDevice();
  if (!node)
    return;

  for (auto& component : GetDataVariables(node))
    components_.emplace_back(ToString16(component.display_name()), component);

  SortNamedNodes(components_);

  for (auto& p : components_)
    components_list_box_.AddString(p.first.c_str());
}

NodeRef AddServiceItemsDialog::GetSelectedDevice() const {
  int i = devices_combo_box_.GetCurSel();
  if (i >= 0 && i < devices_combo_box_.GetCount())
    return devices_[i].second;
  else
    return {};
}

void AddServiceItemsDialog::OnItemChanged(framework::ComboBox& sender,
                                          int old_index,
                                          int new_index) {
  FillChannelsList();
}

void ShowAddServiceItemsDialog(NodeService& node_service,
                               TaskManager& task_manager,
                               const scada::NodeId& node_id) {
  AddServiceItemsDialog dialog{node_service, task_manager, node_id};
  if (dialog.Execute() != IDOK)
    return;
}
