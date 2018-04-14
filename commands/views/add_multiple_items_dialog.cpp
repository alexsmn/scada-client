#include "commands/add_multiple_items_dialog.h"

#include "base/strings/stringprintf.h"
#include "base/strings/sys_string_conversions.h"
#include "common/formula_util.h"
#include "common/node_id_util.h"
#include "common/node_service.h"
#include "common/node_util.h"
#include "common/scada_node_ids.h"
#include "common_resources.h"
#include "services/task_manager.h"
#include "views/client_utils_views.h"
#include "views/framework/control/button.h"
#include "views/framework/control/editbox.h"
#include "views/framework/dialog.h"

#include <atlbase.h>

#include <atlapp.h>
#include <atlctrls.h>

namespace {

void FillDeviceItems(const NodeRef& parent,
                     std::map<base::string16, scada::NodeId>& items) {
  for (auto& node : parent.organizes()) {
    if (IsInstanceOf(node, id::DeviceType)) {
      auto title = GetFullDisplayName(node);
      items.emplace(std::move(title), node.id());
      FillDeviceItems(node, items);
    }
  }
}

}  // namespace

class AddMultipleItemsDialog : public framework::Dialog,
                               private framework::ButtonController,
                               private framework::EditBoxController {
 public:
  AddMultipleItemsDialog(NodeService& node_service,
                         TaskManager& task_manager,
                         const scada::NodeId& group_id);

 protected:
  virtual void OnInitDialog();
  virtual void OnOK();

 private:
  void SetAutoName();

  // framework::ButtonController
  virtual void OnButtonPressed(framework::Button& sender);

  // framework::EditBox
  virtual void OnEditBoxChanged(framework::EditBox& sender);

  NodeService& node_service_;
  TaskManager& task_manager_;
  const scada::NodeId group_id_;

  framework::EditBox name_edit_;
  framework::Button ts_check_;
  framework::Button tit_check_;

  WTL::CComboBox devices_combo_box_;
  std::map<base::string16, scada::NodeId> device_items_;

  bool ts_ = true;
  bool auto_name_ = true;
};

AddMultipleItemsDialog::AddMultipleItemsDialog(NodeService& node_service,
                                               TaskManager& task_manager,
                                               const scada::NodeId& group_id)
    : Dialog{IDD_NEW_ITEMS},
      node_service_{node_service},
      task_manager_{task_manager},
      group_id_{std::move(group_id)} {}

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

  FillDeviceItems(node_service_.GetNode(id::Devices), device_items_);
  for (auto& p : device_items_)
    devices_combo_box_.AddString(p.first.c_str());
  devices_combo_box_.SetCurSel(0);

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

void AddMultipleItemsDialog::OnOK() {
  base::string16 device_desc = win_util::GetWindowText(devices_combo_box_);
  auto i = device_items_.find(device_desc);
  auto device_id = i == device_items_.end() ? scada::NodeId() : i->second;

  auto name_prefix = GetItemText(IDC_NAME_PREFIX);
  auto path_prefix = base::SysWideToNativeMB(GetItemText(IDC_PATH_PREFIX));
  int number = GetItemInt(IDC_STARTING_NUMBER);
  int address = GetItemInt(IDC_STARTING_ADDRESS);
  int count = GetItemInt(IDC_COUNT);

  scada::NodeId type_node_id = ts_ ? id::DiscreteItemType : id::AnalogItemType;

  for (int i = 0; i < count; ++i, ++number, ++address) {
    auto display_name = scada::ToLocalizedText(
        base::StringPrintf(L"%ls%d", name_prefix.c_str(), number));
    auto item_path = base::StringPrintf("%s%d", path_prefix.c_str(), address);
    auto path = MakeNodeIdFormula(MakeNestedNodeId(device_id, item_path));

    task_manager_.PostInsertTask(
        scada::NodeId(), group_id_, type_node_id,
        scada::NodeAttributes().set_display_name(std::move(display_name)),
        {{id::DataItemType_Input1, std::move(path)}});
  }

  __super::OnOK();
}

void AddMultipleItemsDialog::SetAutoName() {
  const wchar_t* name = ts_ ? L"ТС" : L"ТИТ";
  name_edit_.SetText(name);
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
                                TaskManager& task_manager,
                                const scada::NodeId& node_id) {
  AddMultipleItemsDialog dialog{node_service, task_manager, node_id};
  if (dialog.Execute() != IDOK)
    return;
}
