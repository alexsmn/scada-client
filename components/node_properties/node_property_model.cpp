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
                     base::SysNativeMBToWide(node_.browse_name().name()),
                     scada::AttributeId::BrowseName,
                 },
                 {
                     L"(Имя)",
                     ToString16(node_.display_name()),
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
      prop.string_value = def->GetText(*this, node_, prop_decl.id());
      prop.def = def;
      prop.prop_type_id = p.first.id();
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
  return properties_[index].string_value;
}

bool NodePropertyModel::IsInherited(int index) {
  return false;
}

void NodePropertyModel::SetValue(int index, const base::string16& value) {
  auto& prop = properties_[index];
  prop.def->SetText(*this, node_, prop.prop_type_id, value);
}

void NodePropertyModel::OnModelChanged(const scada::ModelChangeEvent& event) {
  if (event.verb & scada::ModelChangeEvent::NodeDeleted) {
    if (node_.id() == event.node_id) {
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
  for (auto attribute_id :
       {scada::AttributeId::BrowseName, scada::AttributeId::DisplayName}) {
    int index = FindProperty(attribute_id);
    if (index != -1) {
      auto& prop = properties_[index];
      prop.string_value = ToString16(
          node_.attribute(attribute_id).get_or(scada::LocalizedText{}));
      TreeNodeChanged(&prop);
    }
  }

  for (auto& prop : properties_) {
    prop.string_value = prop.def->GetText(*this, node_, prop.prop_type_id);
    TreeNodeChanged(&prop);
  }
}

int NodePropertyModel::FindProperty(const scada::NodeId& prop_type_id) const {
  for (int i = 0; i < properties_.size(); ++i) {
    if (properties_[i].prop_type_id == prop_type_id)
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

void* NodePropertyModel::GetParent(void* node) {
  return node == this ? nullptr : this;
}

int NodePropertyModel::GetChildCount(void* parent) {
  if (parent != this)
    return 0;
  return GetCount();
}

void* NodePropertyModel::GetChild(void* parent, int index) {
  if (parent != this)
    return nullptr;
  assert(index < static_cast<int>(properties_.size()));
  return &properties_[index];
}

int NodePropertyModel::NodeToIndex(void* node) const {
  size_t index =
      std::find_if(properties_.begin(), properties_.end(),
                   [node](const Property& prop) { return &prop == node; }) -
      properties_.begin();
  if (index >= properties_.size())
    return -1;
  return static_cast<int>(index);
}

base::string16 NodePropertyModel::GetText(void* node, int column_id) {
  int index = NodeToIndex(node);
  if (index == -1)
    return {};
  return column_id == 0 ? GetName(index) : GetValue(index);
}

void NodePropertyModel::SetText(void* node,
                                int column_id,
                                const base::string16& text) {
  if (column_id != 1)
    return;

  int index = NodeToIndex(node);
  if (index == -1)
    return;

  auto& prop = properties_[index];
  if (prop.def)
    prop.def->SetText(*this, node_, prop.prop_type_id, text);
  else {
    task_manager_.PostUpdateTask(
        node_.id(),
        scada::NodeAttributes().set_browse_name(
            scada::QualifiedName{base::SysWideToNativeMB(text), 0}),
        {});
  }
}
