#include "commands/add_service_items_dialog.h"

#include "base/memory/weak_ptr.h"
#include "base/strings/sys_string_conversions.h"
#include "common/browse_util.h"
#include "common/formula_util.h"
#include "common/node_id_util.h"
#include "common/node_ref.h"
#include "common/node_ref_util.h"
#include "common/node_service.h"
#include "common/scada_node_ids.h"
#include "common_resources.h"
#include "core/configuration_types.h"
#include "services/task_manager.h"
#include "views/framework/control/combobox.h"
#include "views/framework/dialog.h"

namespace scada {
class ViewService;
};

class AddServiceItemsDialog : public framework::Dialog,
                              protected framework::ComboBoxController {
 public:
  AddServiceItemsDialog(scada::ViewService& view_service,
                        NodeService& node_service,
                        const NodeRef& group,
                        TaskManager& task_manager);

 protected:
  // framework::Dialog
  virtual void OnInitDialog();
  virtual void OnOK();

  // framework::ComboBoxController
  virtual void OnItemChanged(framework::ComboBox& sender,
                             int old_index,
                             int new_index);

 private:
  void SetDevices(std::vector<NodeRef> devices);
  void SetComponents(std::vector<NodeRef> components);

  void FillDevicesList();
  void SelectDevice(const scada::NodeId& device_id);
  NodeRef GetSelectedDevice() const;

  void FillChannelsList();

  TaskManager& task_manager_;
  scada::ViewService& view_service_;
  NodeService& node_service_;
  const NodeRef group_;

  std::vector<NodeRef> devices_;
  std::vector<NodeRef> components_;

  WTL::CListBox channels_list_box_;

  framework::ComboBox devices_combo_box_;

  base::WeakPtrFactory<AddServiceItemsDialog> weak_ptr_factory_{this};
};

AddServiceItemsDialog::AddServiceItemsDialog(scada::ViewService& view_service,
                                             NodeService& node_service,
                                             const NodeRef& group,
                                             TaskManager& task_manager)
    : Dialog{IDD_NEW_SERVICE_ITEMS},
      view_service_{view_service},
      node_service_{node_service},
      group_{std::move(group)},
      task_manager_(task_manager) {}

void AddServiceItemsDialog::OnInitDialog() {
  __super::OnInitDialog();

  channels_list_box_ = GetItem(IDC_CHANNELS_LIST);

  devices_combo_box_.Attach(GetItem(IDC_DEVICES_COMBO));
  AttachView(devices_combo_box_, IDC_DEVICES_COMBO);
  devices_combo_box_.SetController(this);

  FillDevicesList();
}

void AddServiceItemsDialog::OnOK() {
  auto device = GetSelectedDevice();

  for (int i = 0; i < channels_list_box_.GetCount(); ++i) {
    if (!channels_list_box_.GetSel(i))
      continue;

    std::string component_name = base::SysWideToNativeMB(
        win_util::GetListBoxItemText(channels_list_box_, i));
    std::string formula =
        MakeNodeIdFormula(MakeNestedNodeId(device.id(), component_name));

    task_manager_.PostInsertTask(
        scada::NodeId(), group_.id(), id::AnalogItemType,
        scada::NodeAttributes().set_browse_name(
            scada::QualifiedName{std::move(component_name), 0}),
        {{id::AnalogItemType_DisplayFormat, "0."},
         {id::DataItemType_Input1, std::move(formula)}});
  }

  __super::OnOK();
}

void AddServiceItemsDialog::FillDevicesList() {
  auto weak_ptr = weak_ptr_factory_.GetWeakPtr();
  BrowseAllDevices(view_service_, node_service_,
                   [weak_ptr](std::vector<NodeRef> devices) {
                     if (auto* ptr = weak_ptr.get())
                       ptr->SetDevices(std::move(devices));
                   });
}

void AddServiceItemsDialog::SelectDevice(const scada::NodeId& device_id) {
  auto i = std::find_if(
      devices_.begin(), devices_.end(),
      [&device_id](const NodeRef& device) { return device.id() == device_id; });
  int index = i == devices_.end() ? -1 : i - devices_.begin();
  devices_combo_box_.SetCurSel(index);

  FillChannelsList();
}

void AddServiceItemsDialog::FillChannelsList() {
  auto device = GetSelectedDevice();
  if (!device) {
    SetComponents({});
    return;
  }

  auto weak_ptr = weak_ptr_factory_.GetWeakPtr();
  BrowseNodes(
      view_service_, node_service_,
      {device.id(), scada::BrowseDirection::Inverse, scada::id::Organizes,
       true},
      [weak_ptr](const scada::Status& status, std::vector<NodeRef> components) {
        auto* ptr = weak_ptr.get();
        if (!ptr)
          return;
        components.erase(std::remove_if(components.begin(), components.end(),
                                        [](const NodeRef& node) {
                                          return node.node_class() !=
                                                 scada::NodeClass::Variable;
                                        }),
                         components.end());
        ptr->SetComponents(std::move(components));
      });
}

void AddServiceItemsDialog::SetComponents(std::vector<NodeRef> components) {
  channels_list_box_.ResetContent();

  components_ = std::move(components);
  for (auto& component : components_)
    channels_list_box_.AddString(
        base::SysNativeMBToWide(component.browse_name().name()).c_str());
}

NodeRef AddServiceItemsDialog::GetSelectedDevice() const {
  int i = devices_combo_box_.GetCurSel();
  if (i == -1)
    return nullptr;
  return devices_[i];
}

void AddServiceItemsDialog::OnItemChanged(framework::ComboBox& sender,
                                          int old_index,
                                          int new_index) {
  FillChannelsList();
}

void AddServiceItemsDialog::SetDevices(std::vector<NodeRef> devices) {
  devices_combo_box_.ResetContent();
  devices_ = std::move(devices);
  for (auto& device : devices_)
    devices_combo_box_.AddString(
        base::SysNativeMBToWide(device.browse_name().name()).c_str());

  //  SelectDevice(id::Server);
}

void ShowAddServiceItemsDialog(scada::ViewService& view_service,
                               NodeService& node_service,
                               const NodeRef& node,
                               TaskManager& task_manager) {
  AddServiceItemsDialog dialog{view_service, node_service, node, task_manager};
  if (dialog.Execute() != IDOK)
    return;
}
