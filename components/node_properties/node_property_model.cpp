#include "components/node_properties/node_property_model.h"

#include "base/strings/sys_string_conversions.h"
#include "node_service/node_service.h"
#include "model/scada_node_ids.h"
#include "services/property_defs.h"
#include "services/task_manager.h"
#include "string_const.h"

#include <algorithm>

NodeGroupModel::NodeGroupModel(NodePropertyModel& property_model)
    : property_model_{property_model} {}

NodeGroupModel::~NodeGroupModel() {}

NodePropertyModel::NodePropertyModel(PropertyContext&& context, NodeRef node)
    : PropertyContext{std::move(context)}, node_{std::move(node)} {
  node_.Fetch(NodeFetchStatus::NodeOnly());
  node_.Subscribe(*this);

  Update();
}

NodePropertyModel::~NodePropertyModel() {
  if (node_)
    node_.Unsubscribe(*this);
}

int NodeGroupModel::GetCount() const {
  return properties.size();
}

PropertyGroup* NodeGroupModel::GetSubgroup(int index) const {
  return properties[index].submodel.get();
}

base::string16 NodeGroupModel::GetName(int index) const {
  return properties[index].name;
}

base::string16 NodeGroupModel::GetValue(int index) const {
  auto& prop = properties[index];
  if (prop.def)
    return prop.def->GetText(property_model_, property_model_.node_,
                             prop.prop_decl_id);
  else
    return ToString16(property_model_.node_.attribute(prop.attribute_id));
}

PropertyGroup::ItemType NodeGroupModel::GetType(int index) const {
  return properties[index].type;
}

bool NodeGroupModel::IsInherited(int index) const {
  return false;
}

void NodeGroupModel::SetValue(int index, const base::string16& value) {
  auto& prop = properties[index];
  if (prop.def)
    prop.def->SetText(property_model_, property_model_.node_, prop.prop_decl_id,
                      value);
  else {
    scada::NodeAttributes attributes;
    // TODO: Other attributes.
    switch (prop.attribute_id) {
      case scada::AttributeId::BrowseName:
        attributes.set_browse_name(ToString(value));
        break;
      case scada::AttributeId::DisplayName:
        attributes.set_display_name(scada::ToLocalizedText(value));
        break;
    }

    if (attributes.empty())
      return;

    property_model_.task_manager_.PostUpdateTask(
        property_model_.node_.node_id(), attributes, {});
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
    if (!root_.properties.empty())
      PropertiesChanged(0, static_cast<int>(root_.properties.size()));
  }
}

void NodePropertyModel::OnNodeSemanticChanged(const scada::NodeId& node_id) {
  if (!root_.properties.empty())
    PropertiesChanged(0, static_cast<int>(root_.properties.size()));
}

void NodePropertyModel::OnNodeFetched(const scada::NodeId& node_id,
                                      bool children) {
  Update();
  if (model_changed_handler)
    model_changed_handler();
}

void NodePropertyModel::Update() {
  root_.properties.clear();

  std::map<NodeRef /*category*/, std::unique_ptr<NodeGroupModel>> groups;
  std::vector<NodeRef> ordered_groups;

  {
    auto& group = groups[{}];
    group = std::make_unique<NodeGroupModel>(*this);
    ordered_groups.emplace_back();

#ifndef NDEBUG
    group->properties.push_back({
        PropertyGroup::ItemType::Property,
        kNodeIdAttributeString.as_string(),
        scada::AttributeId::NodeId,
    });
#endif
    group->properties.push_back({
        PropertyGroup::ItemType::Property,
        kBrowseNameAttributeString.as_string(),
        scada::AttributeId::BrowseName,
    });
    group->properties.push_back({
        PropertyGroup::ItemType::Property,
        kDisplayNameAttributeString.as_string(),
        scada::AttributeId::DisplayName,
    });
  }

  if (const auto& type_definition = node_.type_definition()) {
    for (auto& p : GetTypeProperties(type_definition)) {
      const auto& prop_decl = p.first;
      if (!prop_decl)
        continue;

      const auto& category = prop_decl.target(scada::id::HasPropertyCategory);
      auto& group = groups[category];
      if (!group) {
        group = std::make_unique<NodeGroupModel>(*this);
        ordered_groups.emplace_back(category);
      }

      auto& def = *p.second;

      NodeGroupModel::Property prop;
      prop.type = PropertyGroup::ItemType::Property;
      prop.name = def.GetTitle(*this, prop_decl);
      prop.def = &def;
      prop.prop_decl_id = prop_decl.node_id();

      if (auto* hierarchical_prop = def.AsHierarchical()) {
        prop.type = PropertyGroup::ItemType::Group;
        prop.submodel = std::make_unique<NodeGroupModel>(*this);
        for (auto* child : hierarchical_prop->children()) {
          NodeGroupModel::Property child_prop;
          child_prop.type = PropertyGroup::ItemType::Property;
          child_prop.name = child->GetTitle(*this, prop_decl);
          child_prop.def = child;
          child_prop.prop_decl_id = prop_decl.node_id();
          prop.submodel->properties.emplace_back(std::move(child_prop));
        }
      }

      group->properties.emplace_back(std::move(prop));
    }

    for (auto& category : ordered_groups) {
      auto title = category ? ToString16(category.display_name()) : L"Общие";
      root_.properties.push_back({PropertyGroup::ItemType::Category,
                                  std::move(title), scada::AttributeId::NodeId,
                                  nullptr, scada::NodeId{},
                                  std::move(groups[category])});
    }
  }
}

int NodePropertyModel::FindProperty(const scada::NodeId& prop_decl_id) const {
  auto& properties = root_.properties;
  for (int i = 0; i < properties.size(); ++i) {
    if (properties[i].prop_decl_id == prop_decl_id)
      return i;
  }
  return -1;
}

int NodePropertyModel::FindProperty(scada::AttributeId attribute_id) const {
  auto& properties = root_.properties;
  for (int i = 0; i < properties.size(); ++i) {
    if (properties[i].attribute_id == attribute_id)
      return i;
  }
  return -1;
}

void NodePropertyModel::PropertiesChanged(int first, int count) {
  if (properties_changed_handler)
    properties_changed_handler(root_, first, count);
}

ui::EditData NodeGroupModel::GetEditData(int index) const {
  auto& prop = properties[index];
  if (prop.def)
    return prop.def->GetPropertyEditor(property_model_, property_model_.node_,
                                       prop.prop_decl_id);
  else
    return {};
}
