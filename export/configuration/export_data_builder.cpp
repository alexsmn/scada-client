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

  std::vector<ExportData::PropertyValue> property_values;
  for (const auto& prop : props) {
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

  return ExportData::Node{
      node.node_id(),
      node.parent().node_id(),
      type ? type.display_name() : scada::LocalizedText{},
      type ? type.node_id() : scada::NodeId{},
      node.display_name(),
      std::move(property_values),
  };
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

  return make_all_promise(std::move(node_list_promises))
      .then(
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
    if (auto supertype = type.supertype())
      CollectProperties(supertype, properties, true);
  }

  for (auto p : type.targets(scada::id::HasProperty))
    properties.emplace_back(MakeExportProperty(p));
  for (auto r : type.references(scada::id::NonHierarchicalReferences))
    properties.emplace_back(MakeExportProperty(r.reference_type));
}

}  // namespace

promise<ExportData> ExportDataBuilder::Build() {
  std::vector<ExportData::Property> props;
  CollectProperties(node_service_.GetNode(data_items::id::DiscreteItemType),
                    props, true);
  CollectProperties(node_service_.GetNode(data_items::id::AnalogItemType),
                    props, false);

  auto data_items = node_service_.GetNode(data_items::id::DataItems);
  return FetchNode(data_items).then([data_items, props] {
    return CollectNodeHierarchy(data_items, props)
        .then([props](const std::vector<ExportData::Node>& nodes) {
          return ExportData{std::move(props), std::move(nodes)};
        });
  });
}
