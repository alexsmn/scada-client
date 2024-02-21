#include "export/configuration/export_data_builder.h"

#include "base/range_util.h"
#include "export/configuration/export_data.h"
#include "model/data_items_node_ids.h"
#include "model/node_id_util.h"
#include "node_service/node_promises.h"
#include "node_service/node_service.h"

#include <boost/range/combine.hpp>

namespace {

ExportData::Node MakeExportNode(
    const NodeRef& node,
    const std::vector<ExportData::Property>& props) {
  auto type = node.type_definition();

  std::vector<ExportData::PropertyValue> prop_values;
  for (const auto& prop : props) {
    if (prop.reference) {
      if (auto target = node.target(prop.prop_decl_id)) {
        prop_values.push_back(ExportData::PropertyValue{
            .prop_decl_id = prop.prop_decl_id,
            .target_id = target.node_id(),
            .target_display_name = target.display_name(),
            .reference = true});
      }
    } else {
      if (auto prop_value = node[prop.prop_decl_id].value();
          !prop_value.is_null()) {
        prop_values.emplace_back(ExportData::PropertyValue{
            .prop_decl_id = prop.prop_decl_id, .value = std::move(prop_value)});
      }
    }
  }

  return ExportData::Node{
      .node_id = node.node_id(),
      .parent_id = node.parent().node_id(),
      .type_display_name = type ? type.display_name() : scada::LocalizedText{},
      .type_id = type ? type.node_id() : scada::NodeId{},
      .display_name = node.display_name(),
      .property_values = std::move(prop_values)};
}

promise<std::vector<ExportData::Node>> CollectNodeHierarchy(
    const NodeRef& parent_node,
    const std::vector<ExportData::Property>& props) {
  std::vector<promise<ExportData::Node>> node_promises;
  std::vector<promise<std::vector<ExportData::Node>>> node_list_promises;

  for (const auto& node : parent_node.targets(scada::id::Organizes)) {
    node_promises.emplace_back(FetchNode(node).then(
        [node, &props] { return MakeExportNode(node, props); }));
    node_list_promises.emplace_back(CollectNodeHierarchy(node, props));
  }

  node_list_promises.emplace_back(make_all_promise(std::move(node_promises)));

  return make_all_promise(std::move(node_list_promises))
      .then(
          // Cannot use `&Join` because it cannot pick the proper overload.
          [](const std::vector<std::vector<ExportData::Node>>& node_list_list) {
            return Join(node_list_list);
          });
}

ExportData::Property MakeExportProperty(const NodeRef& node) {
  return {
      node.node_id(),
      node.display_name(),
      node.node_class() == scada::NodeClass::ReferenceType,
  };
}

void CollectProperties(const NodeRef& type,
                       std::vector<ExportData::Property>& properties,
                       bool recursive) {
  if (recursive) {
    if (const auto& supertype = type.supertype()) {
      CollectProperties(supertype, properties, true);
    }
  }

  for (const auto& prop : type.targets(scada::id::HasProperty)) {
    properties.emplace_back(MakeExportProperty(prop));
  }

  for (const auto& ref :
       type.references(scada::id::NonHierarchicalReferences)) {
    properties.emplace_back(MakeExportProperty(ref.reference_type));
  }
}

}  // namespace

promise<ExportData> ExportDataBuilder::Build() const {
  auto props = CollectProps();
  auto root_node = node_service_.GetNode(data_items::id::DataItems);
  return FetchNode(root_node)
      .then(std::bind_front(&CollectNodeHierarchy, root_node, props))
      .then([props](const std::vector<ExportData::Node>& nodes) {
        // TODO: Avoid copying nodes.
        return ExportData{props, nodes};
      });
}

std::vector<ExportData::Property> ExportDataBuilder::CollectProps() const {
  std::vector<ExportData::Property> props;
  CollectProperties(node_service_.GetNode(data_items::id::DiscreteItemType),
                    props, true);
  CollectProperties(node_service_.GetNode(data_items::id::AnalogItemType),
                    props, false);
  return props;
}