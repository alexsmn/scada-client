#include "commands/add_multiple_items_dialog.h"

#include "base/memory/weak_ptr.h"
#include "base/strings/stringprintf.h"
#include "base/strings/sys_string_conversions.h"
#include "common/formula_util.h"
#include "common/node_id_util.h"
#include "common/node_ref.h"
#include "common/node_ref_util.h"
#include "common/scada_node_ids.h"
#include "common_resources.h"
#include "services/task_manager.h"
#include "views/client_utils_views.h"
#include "views/framework/control/button.h"
#include "views/framework/control/editbox.h"
#include "views/framework/dialog.h"

#include <atlapp.h>
#include <atlbase.h>
#include <atlctrls.h>

class AddMultipleItemsDialog : public framework::Dialog,
                               private framework::ButtonController,
                               private framework::EditBoxController {
 public:
  AddMultipleItemsDialog(NodeService& node_service,
                         NodeRef group,
                         TaskManager& task_manager);

 protected:
  virtual void OnInitDialog();
  virtual void OnOK();

 private:
  void SetAutoName();

  void SetDevices(std::vector<NodeRef> devices);

  NodeRef GetSelectedDevice() const;

  // framework::ButtonController
  virtual void OnButtonPressed(framework::Button& sender) override;

  // framework::EditBox
  virtual void OnEditBoxChanged(framework::EditBox& sender) override;

  NodeService& node_service_;
  const NodeRef group_;
  TaskManager& task_manager_;

  framework::EditBox name_edit_;
  framework::Button ts_check_;
  framework::Button tit_check_;

  WTL::CComboBox devices_combo_box_;
  std::vector<NodeRef> devices_;

  bool ts_;
  bool auto_name_;

  base::WeakPtrFactory<AddMultipleItemsDialog> weak_ptr_factory_{this};
};

AddMultipleItemsDialog::AddMultipleItemsDialog(NodeService& node_service,
                                               NodeRef group,
                                               TaskManager& task_manager)
    : Dialog{IDD_NEW_ITEMS},
      node_service_{node_service},
      group_{std::move(group)},
      task_manager_(task_manager),
      ts_(true),
      auto_name_(true) {}

void AddMultipleItemsDialog::OnInitDialog() {
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

  devices_combo_box_ = GetDlgItem(window_handle(), IDC_DEVICES_COMBO);

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

  auto weak_ptr = weak_ptr_factory_.GetWeakPtr();
  BrowseAllDevices(node_service_, [weak_ptr](std::vector<NodeRef> devices) {
    if (auto* ptr = weak_ptr.get())
      ptr->SetDevices(std::move(devices));
  });
}

void AddMultipleItemsDialog::SetDevices(std::vector<NodeRef> devices) {
  devices_combo_box_.ResetContent();
  devices_ = std::move(devices);
  for (auto& device : devices_)
    devices_combo_box_.AddString(
        base::SysNativeMBToWide(device.browse_name().name()).c_str());
}

NodeRef AddMultipleItemsDialog::GetSelectedDevice() const {
  int i = devices_combo_box_.GetCurSel();
  if (i == -1)
    return nullptr;
  return devices_[i];
}

void AddMultipleItemsDialog::OnOK() {
  auto device = GetSelectedDevice();

  std::string name_prefix =
      base::SysWideToNativeMB(GetItemText(IDC_NAME_PREFIX));
  std::string path_prefix =
      base::SysWideToNativeMB(GetItemText(IDC_PATH_PREFIX));

  int number = GetItemInt(IDC_STARTING_NUMBER);
  int address = GetItemInt(IDC_STARTING_ADDRESS);
  int count = GetItemInt(IDC_COUNT);

  scada::NodeId type_node_id = ts_ ? id::DiscreteItemType : id::AnalogItemType;

  for (int i = 0; i < count; ++i, ++number, ++address) {
    std::string name = base::StringPrintf("%s%d", name_prefix.c_str(), number);
    std::string item_path =
        base::StringPrintf("%s%d", path_prefix.c_str(), address);
    std::string path =
        MakeNodeIdFormula(MakeNestedNodeId(device.id(), item_path));

    task_manager_.PostInsertTask(scada::NodeId(), group_.id(), type_node_id,
                                 scada::NodeAttributes().set_browse_name(
                                     scada::QualifiedName{std::move(name), 0}),
                                 {{id::DataItemType_Input1, std::move(path)}});
  }

  __super::OnOK();
}

void AddMultipleItemsDialog::SetAutoName() {
  const char* name = ts_ ? "ря" : "рхр";
  name_edit_.SetText(base::SysNativeMBToWide(name).c_str());
}

void AddMultipleItemsDialog::OnButtonPressed(framework::Button& sender) {
  ts_ = ts_check_.IsChecked();
  if (auto_name_)
    SetAutoName();
}

void AddMultipleItemsDialog::OnEditBoxChanged(framework::EditBox& sender) {
  auto_name_ = false;
}

void ShowAddMultipleItemsDialog(NodeService& node_service,
                                const NodeRef& node,
                                TaskManager& task_manager) {
  AddMultipleItemsDialog dialog{node_service, node, task_manager};
  if (dialog.Execute() != IDOK)
    return;
}
