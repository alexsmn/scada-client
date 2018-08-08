#include "components/node_properties/node_property_model.h"

#include "base/strings/sys_string_conversions.h"
#include "common/node_service.h"
#include "services/property_defs.h"
#include "services/task_manager.h"

#include <algorithm>

NodePropertyModel::NodePropertyModel(PropertyContext&& context, NodeRef node)
    : PropertyContext{std::move(context)}, node_{std::move(node)} {
  node_.Subscribe(*this);

  properties_ = {{
                     L"(Идентификатор)",
                     scada::AttributeId::NodeId,
                 },
                 {
                     L"(Имя)",
                     scada::AttributeId::BrowseName,
                 },
                 {
                     L"(Описание)",
                     scada::AttributeId::DisplayName,
                 }};

  if (const auto& type_definition = node_.type_definition()) {
    for (auto& p : GetTypeProperties(type_definition)) {
      const auto& prop_decl = p.first;
      if (!prop_decl)
        continue;
      auto* def = p.second;
      Property prop;
      prop.name = def->GetTitle(*this, prop_decl);
      prop.def = def;
      prop.prop_decl_id = prop_decl.node_id();
      properties_.emplace_back(std::move(prop));
    }
  }
}

NodePropertyModel::~NodePropertyModel() {
  if (node_)
    node_.Unsubscribe(*this);
}

int NodePropertyModel::GetCount() {
  return properties_.size();
}

base::string16 NodePropertyModel::GetName(int index) {
  return properties_[index].name;
}

base::string16 NodePropertyModel::GetValue(int index) {
  auto& prop = properties_[index];
  if (prop.def)
    return prop.def->GetText(*this, node_, prop.prop_decl_id);
  else
    return ToString16(node_.attribute(prop.attribute_id));
}

bool NodePropertyModel::IsInherited(int index) {
  return false;
}

void NodePropertyModel::SetValue(int index, const base::string16& value) {
  auto& prop = properties_[index];
  if (prop.def)
    prop.def->SetText(*this, node_, prop.prop_decl_id, value);
  else {
    // TODO: attribute id.
    task_manager_.PostUpdateTask(
        node_.node_id(),
        scada::NodeAttributes().set_browse_name(
            scada::QualifiedName{base::SysWideToNativeMB(value), 0}),
        {});
  }
}

void NodePropertyModel::OnModelChanged(const scada::ModelChangeEvent& event) {
  if (event.verb & scada::ModelChangeEvent::NodeDeleted) {
    if (node_.node_id() == event.node_id) {
      node_.Unsubscribe(*this);
      node_ = nullptr;
    }
    // TODO: Model changed?

  } else if (event.verb & (scada::ModelChangeEvent::ReferenceAdded |
                           scada::ModelChangeEvent::ReferenceDeleted)) {
    Update();
  }
}

void NodePropertyModel::OnNodeSemanticChanged(const scada::NodeId& node_id) {
  Update();
}

void NodePropertyModel::Update() {
  PropertiesChanged(0, static_cast<int>(properties_.size()));
}

int NodePropertyModel::FindProperty(const scada::NodeId& prop_decl_id) const {
  for (int i = 0; i < properties_.size(); ++i) {
    if (properties_[i].prop_decl_id == prop_decl_id)
      return i;
  }
  return -1;
}

int NodePropertyModel::FindProperty(scada::AttributeId attribute_id) const {
  for (int i = 0; i < properties_.size(); ++i) {
    if (properties_[i].attribute_id == attribute_id)
      return i;
  }
  return -1;
}

void* NodePropertyTreeModel::GetParent(void* node) {
  return node == this ? nullptr : this;
}

int NodePropertyTreeModel::GetChildCount(void* parent) {
  if (parent != this)
    return 0;
  return GetCount();
}

void* NodePropertyTreeModel::GetChild(void* parent, int index) {
  if (parent != this)
    return nullptr;
  return IndexToNode(index);
}

base::string16 NodePropertyTreeModel::GetText(void* node, int column_id) {
  int index = NodeToIndex(node);
  if (index == -1)
    return {};
  return column_id == 0 ? GetName(index) : GetValue(index);
}

void NodePropertyTreeModel::SetText(void* node,
                                    int column_id,
                                    const base::string16& text) {
  if (column_id != 1)
    return;

  int index = NodeToIndex(node);
  if (index == -1)
    return;

  SetValue(index, text);
}

void NodePropertyModel::PropertiesChanged(int first, int index) {}

void NodePropertyTreeModel::PropertiesChanged(int first, int count) {
  for (int i = 0; i < count; ++i)
    TreeNodeChanged(IndexToNode(first + i));
}

NodePropertyTreeModel::NodePropertyTreeModel(PropertyContext&& context,
                                             NodeRef node)
    : NodePropertyModel{std::move(context), std::move(node)} {}

int NodePropertyTreeModel::NodeToIndex(void* node) const {
  return reinterpret_cast<int>(node) - 1;
}

void* NodePropertyTreeModel::IndexToNode(int index) const {
  return reinterpret_cast<void*>(index + 1);
}

base::string16 NodePropertyTreeModel::GetColumnText(int column_id) const {
  return column_id == 0 ? L"Свойство" : L"Значение";
}

bool NodePropertyTreeModel::IsEditable(void* node, int column_id) const {
  return column_id == 1;
}
