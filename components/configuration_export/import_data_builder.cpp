#include "import_data_builder.h"

#include "base/strings/stringprintf.h"
#include "base/strings/sys_string_conversions.h"
#include "components/configuration_export/export_data.h"
#include "components/configuration_export/resource_error.h"
#include "model/data_items_node_ids.h"
#include "model/node_id_util.h"
#include "node_service/node_service.h"
#include "node_service/node_util.h"

#include <algorithm>
#include <set>

namespace {

void ScanDeleteNodes(const NodeRef& parent_node,
                     const scada::NodeId& type_id,
                     const std::set<scada::NodeId>& exclude_ids,
                     std::vector<scada::NodeId>& results) {
  for (const auto& node : parent_node.targets(scada::id::Organizes)) {
    if (IsInstanceOf(node, type_id)) {
      if (exclude_ids.find(node.node_id()) == exclude_ids.end())
        results.emplace_back(node.node_id());
    }
    ScanDeleteNodes(node, type_id, exclude_ids, results);
  }
}

}  // namespace

ImportData BuildImportData(NodeService& node_service,
                           const ExportData& export_data) {
  ImportData import_data;

  std::set<scada::NodeId> listed_nodes;

  for (const auto& export_node : export_data.nodes) {
    auto node = node_service.GetNode(export_node.node_id);
    if (node)
      listed_nodes.emplace(node.node_id());

    auto type_definition = node_service.GetNode(export_node.type_id);
    if (!type_definition) {
      throw ResourceError{base::StringPrintf(
          L"“ип %ls не найден",
          base::SysNativeMBToWide(NodeIdToScadaString(export_node.type_id))
              .c_str())};
    }

    scada::NodeAttributes attrs;
    if (!node || node.display_name() != export_node.display_name)
      attrs.set_display_name(std::move(export_node.display_name));

    // Props & refs.
    scada::NodeProperties props;
    std::vector<ImportData::Reference> refs;

    for (const auto& export_value : export_node.property_values) {
      if (export_value.reference) {
        auto old_target_id = node.target(export_value.node_id).node_id();
        if (old_target_id != export_value.target_id) {
          refs.push_back(
              {export_value.node_id, old_target_id, export_value.target_id});
        }
      } else {
        if (node) {
          auto value = node[export_value.node_id].value();
          if (value == export_value.value)
            continue;
        }

        props.emplace_back(export_value.node_id, export_value.value);
      }
    }

    if (node) {
      if (!attrs.empty() || !props.empty() || !refs.empty()) {
        import_data.modify_nodes.push_back(
            {export_node.node_id, type_definition.node_id(),
             export_node.parent_id, std::move(attrs), std::move(props),
             std::move(refs)});
      }
    } else {
      import_data.create_nodes.push_back(
          {export_node.node_id, type_definition.node_id(),
           export_node.parent_id, std::move(attrs), std::move(props),
           std::move(refs)});
    }
  }

  ScanDeleteNodes(node_service.GetNode(data_items::id::DataItems),
                  data_items::id::DataItemType, listed_nodes,
                  import_data.delete_nodes);

  return import_data;
}
