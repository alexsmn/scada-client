#include "components/configuration_export/export_data_collector.h"

#include "model/data_items_node_ids.h"
#include "model/node_id_util.h"
#include "node_service/node_service.h"

namespace {

ExportData::Node MakeExportNode(
    const NodeRef& node,
    const std::vector<ExportData::Property>& props) {
  auto type = node.type_definition();

  std::vector<ExportData::PropertyValue> property_values;
  for (auto prop : props) {
    if (prop.reference) {
      auto target = node.target(prop.node_id);
      property_values.push_back(
          {prop.node_id, {}, target.node_id(), target.display_name(), true});
    } else {
      auto value = node[prop.node_id].value();
      property_values.push_back(
          {prop.node_id, std::move(value), {}, {}, false});
    }
  }
  return {
      node.node_id(),
      node.parent().node_id(),
      type ? type.display_name() : scada::LocalizedText{},
      type ? type.node_id() : scada::NodeId{},
      node.display_name(),
      std::move(property_values),
  };
}

void CollectNodes(const NodeRef& parent_node,
                  const std::vector<ExportData::Property>& props,
                  std::vector<ExportData::Node>& nodes) {
  for (const auto& node : parent_node.targets(scada::id::Organizes)) {
    nodes.emplace_back(MakeExportNode(node, props));
    CollectNodes(node, props, nodes);
  }
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
    if (auto supertype = type.supertype())
      CollectProperties(supertype, properties, true);
  }

  for (auto p : type.targets(scada::id::HasProperty))
    properties.emplace_back(MakeExportProperty(p));
  for (auto r : type.references())
    properties.emplace_back(MakeExportProperty(r.reference_type));
}

}

ExportData CollectExportData(NodeService& node_service) {
  std::vector<ExportData::Property> props;
  CollectProperties(node_service.GetNode(data_items::id::DiscreteItemType),
                    props, true);
  CollectProperties(node_service.GetNode(data_items::id::AnalogItemType), props,
                    false);

  std::vector<ExportData::Node> nodes;
  CollectNodes(node_service.GetNode(data_items::id::DataItems), props, nodes);

  return {std::move(props), std::move(nodes)};
}
