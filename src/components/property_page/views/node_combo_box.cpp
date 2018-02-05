#include "node_combo_box.h"

#include "base/string_util.h"
#include "base/win/win_util2.h"
#include "common/browse_util.h"
#include "common/node_util.h"

namespace {

const base::char16 kNoneChoice[] = L"<Нет>";

base::string16 MakeDeviceComponentItem(
    const scada::LocalizedText& display_name) {
  return L'<' + ToString16(display_name) + L'>';
}

}  // namespace

// NodeComboBox

void NodeComboBox::Init(HWND combo_box_handle) {
  combo_box_ = combo_box_handle;
}

void NodeComboBox::Fill(scada::ViewService& view_service,
                        NodeService& node_service,
                        const scada::NodeId& root_node_id,
                        const scada::NodeId& type_definition_id,
                        const scada::NodeId& selected_node_id) {
  nodes_.clear();
  combo_box_.ResetContent();
  auto weak_ptr = weak_ptr_factory_.GetWeakPtr();
  BrowseNodesRecursive(
      view_service, node_service, root_node_id, type_definition_id,
      [weak_ptr, this, selected_node_id](const std::vector<NodeRef>& nodes) {
        if (!weak_ptr.get())
          return;
        for (auto& node : nodes) {
          auto name = ToString16(node.display_name());
          nodes_.emplace(name, node);
          combo_box_.AddString(name.c_str());
        }
        Select(selected_node_id);
      });
}

bool NodeComboBox::Select(const scada::NodeId& node_id) {
  bool ok = false;
  base::string16 choice = kNoneChoice;
  for (auto& p : nodes_) {
    if (p.second.id() == node_id) {
      choice = p.first;
      ok = true;
      break;
    }
  }
  combo_box_.SelectString(-1, choice.c_str());
  return ok;
}

NodeRef NodeComboBox::GetSelection() const {
  int selected_index = combo_box_.GetCurSel();
  if (selected_index == -1)
    return {};
  auto choice = win_util::GetComboBoxItemText(combo_box_, selected_index);
  if (IsEqualNoCase(choice, kNoneChoice))
    return {};
  auto i = nodes_.find(choice);
  return i == nodes_.end() ? NodeRef{} : i->second;
}

// ItemComboBox

void ItemComboBox::Init(HWND combo_box_handle) {
  combo_box_ = combo_box_handle;
}

scada::NodeId ItemComboBox::GetSelectedId() const {
  base::string16 item = win_util::GetWindowText(combo_box_);
  auto i = component_items_.find(item);
  return i != component_items_.end() ? i->second : scada::NodeId{};
}

void ItemComboBox::Fill(scada::ViewService& view_service,
                        NodeService& node_service,
                        const scada::NodeId& device_id) {
  combo_box_.ResetContent();
  component_items_.clear();

  // add service items
  auto weak_ptr = weak_ptr_factory_.GetWeakPtr();
  BrowseNodes(view_service, node_service,
              {device_id, scada::BrowseDirection::Forward,
               scada::id::HasComponent, true},
              [weak_ptr, this](const scada::Status& status,
                               const std::vector<NodeRef>& components) {
                if (!status || !weak_ptr.get())
                  return;
                for (auto& component : components) {
                  if (component.node_class() != scada::NodeClass::Variable)
                    continue;
                  auto name = MakeDeviceComponentItem(component.display_name());
                  component_items_.emplace(name, component.id());
                  combo_box_.AddString(name.c_str());
                }
              });
}

base::string16 ItemComboBox::GetText() const {
  return win_util::GetWindowText(combo_box_);
}

void ItemComboBox::SetText(base::StringPiece16 text) {
  combo_box_.SetWindowText(text.as_string().c_str());
}
