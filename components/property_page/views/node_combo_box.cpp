#include "node_combo_box.h"

#include "base/string_util.h"
#include "base/strings/sys_string_conversions.h"
#include "base/win/win_util2.h"
#include "model/node_id_util.h"
#include "node_service/node_service.h"
#include "node_service/node_util.h"
#include "core/standard_node_ids.h"

namespace {
const base::char16 kNoneChoice[] = L"<Нет>";
}  // namespace

// NodeComboBox

void NodeComboBox::Init(HWND combo_box_handle) {
  combo_box_ = combo_box_handle;
}

void NodeComboBox::Fill(const NodeRef& root,
                        const scada::NodeId& type_definition_id,
                        const scada::NodeId& selected_node_id) {
  combo_box_.ResetContent();

  nodes_ = GetNamedNodes(root, type_definition_id);
  SortNamedNodes(nodes_);
  nodes_.insert(nodes_.begin(), {kNoneChoice, nullptr});

  for (auto& p : nodes_)
    combo_box_.AddString(p.first.c_str());

  Select(selected_node_id);
}

bool NodeComboBox::Select(const scada::NodeId& node_id) {
  for (size_t i = 0; i < nodes_.size(); ++i) {
    if (nodes_[i].second.node_id() == node_id) {
      combo_box_.SetCurSel(static_cast<int>(i));
      return true;
    }
  }
  return false;
}

NodeRef NodeComboBox::GetSelection() const {
  int index = combo_box_.GetCurSel();
  if (index >= 0 && index < static_cast<int>(nodes_.size()))
    return nodes_[index].second;
  else
    return nullptr;
}

// ItemComboBox

void ItemComboBox::Init(NodeService& node_service, HWND combo_box_handle) {
  node_service_ = &node_service;
  combo_box_ = combo_box_handle;
}

scada::NodeId ItemComboBox::GetNodeId() const {
  base::string16 text = win_util::GetWindowText(combo_box_);
  auto i = items_.find(text);
  if (i != items_.end())
    return i->second;

  return MakeNestedNodeId(device_id_, base::SysWideToNativeMB(text));
}

void ItemComboBox::SetDeviceId(const scada::NodeId& device_id) {
  combo_box_.SetWindowTextW(L"");
  combo_box_.ResetContent();
  items_.clear();
  node_ids_.clear();

  device_id_ = device_id;

  AddNodesRecursive(device_id_, {scada::id::Organizes, scada::id::HasComponent},
                    {});
}

void ItemComboBox::SetNodeId(const scada::NodeId& node_id) {
  auto i = node_ids_.find(node_id);
  if (i != node_ids_.end()) {
    combo_box_.SetWindowTextW(i->second.c_str());
    return;
  }

  scada::NodeId device_id;
  base::StringPiece nested_name;
  IsNestedNodeId(node_id, device_id, nested_name);

  assert(device_id == device_id_);

  combo_box_.SetWindowTextW(base::SysNativeMBToWide(nested_name).c_str());
}

void ItemComboBox::AddNodesRecursive(
    const scada::NodeId& parent_id,
    const std::vector<scada::NodeId>& reference_type_ids,
    const base::string16& name_prefix) {
  assert(node_service_);

  for (auto& reference_type_id : reference_type_ids) {
    for (const auto& child :
         node_service_->GetNode(parent_id).targets(reference_type_id)) {
      auto name = name_prefix + ToString16(child.display_name());

      if (child.node_class() == scada::NodeClass::Variable) {
        items_.emplace(name, child.node_id());
        node_ids_.emplace(child.node_id(), name);
        combo_box_.AddString(name.c_str());
      }

      AddNodesRecursive(child.node_id(), reference_type_ids, name + L" : ");
    }
  }
}
