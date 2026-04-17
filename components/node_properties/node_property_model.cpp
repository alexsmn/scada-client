#include "components/node_properties/node_property_model.h"

#include "aui/translation.h"
#include "model/scada_node_ids.h"
#include "node_service/node_promises.h"
#include "node_service/node_service.h"
#include "properties/property_definition.h"
#include "properties/property_service.h"
#include "scada/event.h"
#include "app/string_const.h"

#include <span>

namespace {

void FetchNodes(NodeService& node_service,
                std::span<const scada::NodeId> node_ids) {
  std::ranges::for_each(
      node_ids, [&](auto& node_id) { node_service.GetNode(node_id).Fetch(); });
}

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
  node_service_.Subscribe(*this);

  FetchNode(node_).then(cancelation_.Bind([this] { OnNodeFetched(); }));
}

NodePropertyModel::~NodePropertyModel() {
  node_service_.Unsubscribe(*this);
}

void NodePropertyModel::OnModelChanged(const scada::ModelChangeEvent& event) {
  if (node_.node_id() != event.node_id) {
    return;
  }

  if (event.verb & scada::ModelChangeEvent::NodeDeleted) {
    node_service_.Unsubscribe(*this);
    node_ = nullptr;
    node_deleted();

  } else if (event.verb & (scada::ModelChangeEvent::ReferenceAdded |
                           scada::ModelChangeEvent::ReferenceDeleted)) {
    if (!root_.properties.empty())
      PropertiesChanged(0, static_cast<int>(root_.properties.size()));
  }
}

void NodePropertyModel::OnNodeSemanticChanged(const scada::NodeId& node_id) {
  // TODO: Update only changed nodes and additional targets.
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
    auto& group = groups[Translate("Attributes")];
    group = std::make_unique<NodeGroupModel>(*this);

#ifndef NDEBUG
    group->properties.emplace_back(aui::PropertyGroup::ItemType::Property,
                                   std::u16string{kNodeIdAttributeString},
                                   scada::AttributeId::NodeId);
#endif
    group->properties.emplace_back(aui::PropertyGroup::ItemType::Property,
                                   Translate("Browse Name"),
                                   scada::AttributeId::BrowseName);
    group->properties.emplace_back(aui::PropertyGroup::ItemType::Property,
                                   Translate("Name"),
                                   scada::AttributeId::DisplayName);
  }

  if (const auto& type_definition = node_.type_definition()) {
    assert(type_definition.fetched());

    for (const auto& [prop_decl, prop_def] :
         property_service_.GetTypePropertyDefs(type_definition)) {
      if (!prop_decl)
        continue;

      const auto& category = prop_decl.target(scada::id::HasPropertyCategory);
      auto& group = groups[ToString16(category.display_name())];
      if (!group) {
        group = std::make_unique<NodeGroupModel>(*this);
      }

      NodeGroupModel::Property prop{
          .type = aui::PropertyGroup::ItemType::Property,
          .name = prop_def->GetTitle(*this, prop_decl),
          .def = prop_def,
          .prop_decl_id = prop_decl.node_id()};
      InitProperty(prop);

      if (auto* hierarchical_prop = prop_def->AsHierarchical()) {
        prop.type = aui::PropertyGroup::ItemType::Group;
        prop.submodel = std::make_unique<NodeGroupModel>(*this);
        for (const auto* child : hierarchical_prop->children()) {
          NodeGroupModel::Property child_prop{
              .type = aui::PropertyGroup::ItemType::Property,
              .name = child->GetTitle(*this, prop_decl),
              .def = child,
              .prop_decl_id = prop_decl.node_id()};
          InitProperty(child_prop);
          prop.submodel->properties.emplace_back(std::move(child_prop));
        }
      }

      group->properties.emplace_back(std::move(prop));
    }
  }

  for (auto& [title, group] : groups) {
    auto new_title = title.empty() ? Translate("Misc") : title;
    root_.properties.emplace_back(
        NodeGroupModel::Property{.type = aui::PropertyGroup::ItemType::Category,
                                 .name = std::move(new_title),
                                 .attribute_id = scada::AttributeId::NodeId,
                                 .submodel = std::move(group)});
  }

  SortPropertiesRecursive(root_.properties);
}

void NodePropertyModel::InitProperty(NodeGroupModel::Property& prop) {
  prop.def->GetAdditionalTargets(node_, prop.prop_decl_id,
                                 prop.additional_targets);
  FetchNodes(node_service_, prop.additional_targets);
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
