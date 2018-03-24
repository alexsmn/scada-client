#include "components/node_properties/node_property_model.h"

#include "base/strings/sys_string_conversions.h"
#include "common/node_service.h"
#include "services/property_defs.h"

#include <algorithm>

NodePropertyModel::NodePropertyModel(NodeService& node_service,
                                     TaskManager& task_manager,
                                     Nodes nodes)
    : PropertyContext{node_service, task_manager}, nodes_(std::move(nodes)) {
  for (auto& node : nodes_)
    node.Subscribe(*this);

  auto node = !nodes_.empty() ? nodes_.front() : nullptr;
  auto type = node.type_definition();

  if (type) {
    for (auto& p : GetTypeProperties(type)) {
      auto prop_decl = node_service_.GetNode(p.first);
      assert(prop_decl);
      auto* def = p.second;
      Property prop;
      prop.display_name = prop_decl.display_name();
      prop.string_value = def->GetText(*this, node, p.first);
      prop.def = def;
      prop.prop_type_id = p.first;
      properties_.emplace_back(std::move(prop));
    }
  }
}

NodePropertyModel::~NodePropertyModel() {
  for (auto& node : nodes_)
    node.Subscribe(*this);
}

int NodePropertyModel::GetCount() {
  return properties_.size();
}

base::string16 NodePropertyModel::GetName(int index) {
  return properties_[index].display_name;
}

base::string16 NodePropertyModel::GetValue(int index) {
  return properties_[index].string_value;
}

bool NodePropertyModel::IsInherited(int index) {
  return false;
}

void NodePropertyModel::SetValue(int index, const base::string16& value) {
  auto& prop = properties_[index];
  prop.def->SetText(*this, nodes_.front(), prop.prop_type_id, value);
}

void NodePropertyModel::OnModelChanged(const scada::ModelChangeEvent& event) {
  if (!(event.verb & scada::ModelChangeEvent::NodeDeleted))
    return;

  auto i = std::find_if(nodes_.begin(), nodes_.end(), [&](const NodeRef& node) {
    return node.id() == event.node_id;
  });
  if (i == nodes_.end())
    return;

  i->Unsubscribe(*this);
  nodes_.erase(i);
}

void NodePropertyModel::OnNodeSemanticChanged(const scada::NodeId& node_id) {
  auto node = nodes_.front();
  if (node_id != node.id())
    return;

  for (auto& prop : properties_)
    prop.string_value = prop.def->GetText(*this, node, prop.prop_type_id);

  if (observer_)
    observer_->OnValuesChanged(0, static_cast<int>(properties_.size()));
}

int NodePropertyModel::FindProperty(const scada::NodeId& prop_type_id) const {
  for (int i = 0; i < properties_.size(); ++i) {
    if (properties_[i].prop_type_id == prop_type_id)
      return i;
  }
  return -1;
}
