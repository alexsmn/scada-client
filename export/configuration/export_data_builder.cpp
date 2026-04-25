#include "export/configuration/export_data_builder.h"

#include "base/awaitable_promise.h"
#include "base/executor_conversions.h"
#include "export/configuration/export_data.h"
#include "model/data_items_node_ids.h"
#include "model/node_id_util.h"
#include "node_service/node_promises.h"
#include "node_service/node_service.h"

#include <algorithm>
#include <stdexcept>

namespace {

const scada::NamespaceIndex kExportedNamespaceIndexes[] = {
    NamespaceIndexes::GROUP, NamespaceIndexes::TS, NamespaceIndexes::TIT};

ExportData::Node MakeExportNode(
    const NodeRef& node,
    const std::vector<ExportData::Property>& props) {
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

  const NodeRef& type_def = node.type_definition();

  return ExportData::Node{
      .node_id = node.node_id(),
      .parent_id = node.parent().node_id(),
      .type_display_name =
          type_def ? type_def.display_name() : scada::LocalizedText{},
      .type_id = type_def ? type_def.node_id() : scada::NodeId{},
      .display_name = node.display_name(),
      .property_values = std::move(prop_values)};
}

Awaitable<std::vector<ExportData::Node>> CollectNodeHierarchyAsync(
    AnyExecutor executor,
    const NodeRef& parent_node,
    const std::vector<ExportData::Property>& props) {
  std::vector<ExportData::Node> nodes;

  for (const auto& node : parent_node.targets(scada::id::Organizes)) {
    co_await AwaitPromise(executor, FetchNode(node));
    nodes.emplace_back(MakeExportNode(node, props));

    auto child_nodes =
        co_await CollectNodeHierarchyAsync(executor, node, props);
    nodes.insert(nodes.end(), std::make_move_iterator(child_nodes.begin()),
                 std::make_move_iterator(child_nodes.end()));
  }

  std::erase_if(nodes, [](const ExportData::Node& node) {
    return std::ranges::find(kExportedNamespaceIndexes,
                             node.node_id.namespace_index()) ==
           std::end(kExportedNamespaceIndexes);
  });
  co_return nodes;
}

Awaitable<ExportData> BuildExportDataAsync(
    AnyExecutor executor,
    NodeRef root_node,
    std::vector<ExportData::Property> props) {
  co_await AwaitPromise(executor, FetchNode(root_node));
  auto nodes = co_await CollectNodeHierarchyAsync(executor, root_node, props);
  co_return ExportData{std::move(props), std::move(nodes)};
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
  if (!executor_) {
    return make_rejected_promise<ExportData>(
        std::logic_error{"ExportDataBuilder executor is not configured"});
  }

  return ToPromise(
      MakeAnyExecutor(executor_),
      BuildExportDataAsync(MakeAnyExecutor(executor_),
                           node_service_.GetNode(data_items::id::DataItems),
                           CollectProps()));
}

Awaitable<ExportData> ExportDataBuilder::BuildAsync(
    AnyExecutor executor) const {
  return BuildExportDataAsync(std::move(executor),
                              node_service_.GetNode(data_items::id::DataItems),
                              CollectProps());
}

std::vector<ExportData::Property> ExportDataBuilder::CollectProps() const {
  std::vector<ExportData::Property> props;
  CollectProperties(node_service_.GetNode(data_items::id::DiscreteItemType),
                    props, true);
  CollectProperties(node_service_.GetNode(data_items::id::AnalogItemType),
                    props, false);
  return props;
}
