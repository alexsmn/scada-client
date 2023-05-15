#include "components/node_properties/node_property_model.h"

#include "core/event.h"
#include "model/scada_node_ids.h"
#include "node_service/node_promises.h"
#include "node_service/node_service.h"
#include "services/properties/property_definition.h"
#include "services/properties/property_service.h"
#include "string_const.h"

namespace {

void SortPropertiesRecursive(
    std::vector<NodeGroupModel::Property>& properties) {
  std::ranges::sort(properties, {},
                    [](const NodeGroupModel::Property& p) { return p.name; });

  for (auto& p : properties) {
    if (p.submodel) {
      SortPropertiesRecursive(p.submodel->properties);
    }
  }
}

}  // namespace

// NodePropertyModel

NodePropertyModel::NodePropertyModel(PropertyService& property_service,
                                     PropertyContext&& context,
                                     NodeRef node)
    : PropertyContext{std::move(context)},
      property_service_{property_service},
      node_{std::move(node)} {
  node_.Subscribe(*this);

  FetchNode(node_).then(cancelation_.Bind([this] { OnNodeFetched(); }));
}

NodePropertyModel::~NodePropertyModel() {
  node_.Unsubscribe(*this);
}

void NodePropertyModel::OnModelChanged(const scada::ModelChangeEvent& event) {
  assert(node_.node_id() == event.node_id);

  if (event.verb & scada::ModelChangeEvent::NodeDeleted) {
    node_.Unsubscribe(*this);
    node_ = nullptr;
    node_deleted();

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

void NodePropertyModel::OnNodeFetched() {
  Update();

  if (model_changed_handler)
    model_changed_handler();
}

void NodePropertyModel::Update() {
  root_.properties.clear();

  if (!node_.fetched())
    return;

  std::unordered_map<std::u16string, std::unique_ptr<NodeGroupModel>> groups;

  {
    auto& group = groups[u"Атрибуты"];
    group = std::make_unique<NodeGroupModel>(*this);

#ifndef NDEBUG
    group->properties.emplace_back(aui::PropertyGroup::ItemType::Property,
                                   std::u16string{kNodeIdAttributeString},
                                   scada::AttributeId::NodeId);
#endif
    group->properties.emplace_back(aui::PropertyGroup::ItemType::Property,
                                   std::u16string{kBrowseNameAttributeString},
                                   scada::AttributeId::BrowseName);
    group->properties.emplace_back(aui::PropertyGroup::ItemType::Property,
                                   std::u16string{kDisplayNameAttributeString},
                                   scada::AttributeId::DisplayName);
  }

  if (const auto& type_definition = node_.type_definition()) {
    assert(type_definition.fetched());

    for (auto& p : property_service_.GetTypePropertyDefs(type_definition)) {
      const auto& prop_decl = p.first;
      if (!prop_decl)
        continue;

      const auto& category = prop_decl.target(scada::id::HasPropertyCategory);
      auto& group = groups[ToString16(category.display_name())];
      if (!group) {
        group = std::make_unique<NodeGroupModel>(*this);
      }

      auto& def = *p.second;

      NodeGroupModel::Property prop;
      prop.type = aui::PropertyGroup::ItemType::Property;
      prop.name = def.GetTitle(*this, prop_decl);
      prop.def = &def;
      prop.prop_decl_id = prop_decl.node_id();

      if (auto* hierarchical_prop = def.AsHierarchical()) {
        prop.type = aui::PropertyGroup::ItemType::Group;
        prop.submodel = std::make_unique<NodeGroupModel>(*this);
        for (auto* child : hierarchical_prop->children()) {
          NodeGroupModel::Property child_prop;
          child_prop.type = aui::PropertyGroup::ItemType::Property;
          child_prop.name = child->GetTitle(*this, prop_decl);
          child_prop.def = child;
          child_prop.prop_decl_id = prop_decl.node_id();
          prop.submodel->properties.emplace_back(std::move(child_prop));
        }
      }

      group->properties.emplace_back(std::move(prop));
    }
  }

  for (auto& [title, group] : groups) {
    auto new_title = title.empty() ? u"Разное" : title;
    root_.properties.emplace_back(
        aui::PropertyGroup::ItemType::Category, std::move(new_title),
        scada::AttributeId::NodeId, nullptr, scada::NodeId{}, std::move(group));
  }

  SortPropertiesRecursive(root_.properties);
}

int NodePropertyModel::FindProperty(const scada::NodeId& prop_decl_id) const {
  auto& properties = root_.properties;
  for (size_t i = 0; i < properties.size(); ++i) {
    if (properties[i].prop_decl_id == prop_decl_id)
      return static_cast<int>(i);
  }
  return -1;
}

int NodePropertyModel::FindProperty(scada::AttributeId attribute_id) const {
  auto& properties = root_.properties;
  for (size_t i = 0; i < properties.size(); ++i) {
    if (properties[i].attribute_id == attribute_id)
      return static_cast<int>(i);
  }
  return -1;
}

void NodePropertyModel::PropertiesChanged(int first, int count) {
  if (properties_changed_handler)
    properties_changed_handler(root_, first, count);
}
