#include "components/node_properties/node_property_model.h"

#include "base/strings/sys_string_conversions.h"
#include "common/browse_util.h"
#include "common/node_service.h"
#include "services/property_defs.h"
#include "services/task_manager.h"
#include "translation.h"

NodePropertyModel::NodePropertyModel(const PropertyContext& context,
                                     NodeIds node_ids)
    : context_(context) {
  auto weak_ptr = weak_ptr_factory_.GetWeakPtr();
  RequestNodes(context.node_service_, node_ids,
               [weak_ptr](std::vector<scada::Status> statuses,
                          std::vector<NodeRef> nodes) {
                 if (auto* ptr = weak_ptr.get()) {
                   // TODO: Handle errors.
                   nodes.erase(std::remove(nodes.begin(), nodes.end(), nullptr),
                               nodes.end());
                   ptr->SetNodes(std::move(nodes));
                 }
               });
}

NodePropertyModel::~NodePropertyModel() {
  context_.node_service_.RemoveObserver(*this);
}

void NodePropertyModel::SetNodes(const Nodes& nodes) {
  context_.node_service_.AddObserver(*this);

  for (auto& node : nodes)
    nodes_.emplace(node.id(), node);

  if (!nodes.empty())
    node_ = nodes_.begin()->second;

  auto node = !nodes_.empty() ? nodes_.begin()->second : nullptr;
  const auto& type_definition = node.type_definition();

  if (node) {
    properties_.push_back({
        L"(Имя)",
        base::SysNativeMBToWide(node.browse_name().name()),
        scada::AttributeId::BrowseName,
    });
    properties_.push_back({
        L"(Обозначение)",
        ToString16(node.display_name()),
        scada::AttributeId::DisplayName,
    });
  }

  if (type_definition) {
    for (auto& p : GetTypeProperties(type_definition)) {
      const auto& prop_decl = p.first;
      if (!prop_decl)
        continue;
      auto* def = p.second;
      Property prop;
      prop.name = def->GetTitle(context_, prop_decl);
      prop.string_value = def->GetText(context_, node, prop_decl.id());
      prop.def = def;
      prop.prop_type_id = p.first.id();
      properties_.emplace_back(std::move(prop));
    }
  }
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
  prop.def->SetText(context_, node_, prop.prop_type_id, value);
}

void NodePropertyModel::OnModelChange(const ModelChangeEvent& event) {
  if (event.verb & ModelChangeEvent::NodeDeleted) {
    nodes_.erase(event.node_id);
    if (node_.id() == event.node_id)
      node_ = nullptr;
    // TODO: Model changed?

  } else if (event.verb & (ModelChangeEvent::ReferenceAdded |
                           ModelChangeEvent::ReferenceDeleted)) {
    UpdateNode(event.node_id);
  }
}

void NodePropertyModel::OnNodeSemanticChanged(const scada::NodeId& node_id) {
  UpdateNode(node_id);
}

void NodePropertyModel::UpdateNode(const scada::NodeId& node_id) {
  if (node_id != node_.id())
    return;

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
    prop.string_value = prop.def->GetText(context_, node_, prop.prop_type_id);
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

  for (auto& p : nodes_) {
    auto& prop = properties_[index];
    if (prop.def)
      prop.def->SetText(context_, p.second, prop.prop_type_id, text);
    else {
      context_.task_manager_.PostUpdateTask(
          p.first,
          scada::NodeAttributes().set_browse_name(
              scada::QualifiedName{base::SysWideToNativeMB(text), 0}),
          {});
    }
  }
}
