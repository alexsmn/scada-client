#include "import_data_builder.h"

#include "aui/resource_error.h"
#include "base/strings/stringprintf.h"
#include "base/strings/utf_string_conversions.h"
#include "export/configuration/export_data.h"
#include "model/data_items_node_ids.h"
#include "model/node_id_util.h"
#include "node_service/node_service.h"
#include "node_service/node_util.h"

#include <algorithm>
#include <unordered_set>

class ImportDataBuilder {
 public:
  explicit ImportDataBuilder(NodeService& node_service)
      : node_service_{node_service} {}

  ImportData Run(const ExportData& export_data) && {
    for (const auto& export_node : export_data.nodes) {
      ProcessNode(export_node);
    }

    ScanDeleteNodes(node_service_.GetNode(data_items::id::DataItems),
                    data_items::id::DataItemType);

    return std::move(import_data);
  }

 private:
  void ProcessNode(const ExportData::Node& export_node) {
    auto node = node_service_.GetNode(export_node.node_id);
    assert(node.fetched());
    if (node) {
      listed_nodes.emplace(node.node_id());
    }

    scada::NodeAttributes attrs;
    if (!node || node.display_name() != export_node.display_name) {
      attrs.set_display_name(export_node.display_name);
    }

    // Props & refs.
    scada::NodeProperties props;
    std::vector<ImportData::Reference> refs;

    for (const auto& export_value : export_node.property_values) {
      if (export_value.reference) {
        auto old_target_id = node.target(export_value.prop_decl_id).node_id();
        if (old_target_id != export_value.target_id) {
          refs.emplace_back(export_value.prop_decl_id, old_target_id,
                            export_value.target_id);
        }
      } else {
        if (node) {
          auto value = node[export_value.prop_decl_id].value();
          if (value == export_value.value) {
            continue;
          }
        }

        props.emplace_back(export_value.prop_decl_id, export_value.value);
      }
    }

    if (node) {
      if (!attrs.empty() || !props.empty() || !refs.empty()) {
        import_data.modify_nodes.emplace_back(
            export_node.node_id, export_node.type_id, export_node.parent_id,
            std::move(attrs), std::move(props), std::move(refs));
      }
    } else {
      import_data.create_nodes.emplace_back(
          export_node.node_id, export_node.type_id, export_node.parent_id,
          std::move(attrs), std::move(props), std::move(refs));
    }
  }

  void ScanDeleteNodes(const NodeRef& parent_node,
                       const scada::NodeId& type_id) {
    for (const auto& node : parent_node.targets(scada::id::Organizes)) {
      if (IsInstanceOf(node, type_id) &&
          !listed_nodes.contains(node.node_id())) {
        import_data.delete_nodes.emplace_back(node.node_id());
      }
      ScanDeleteNodes(node, type_id);
    }
  }

  // The node service is used to compare the new configuration snapshot with the
  // existing nodes.
  NodeService& node_service_;

  std::unordered_set<scada::NodeId> listed_nodes;

  ImportData import_data;
};

ImportData BuildImportData(NodeService& node_service,
                           const ExportData& export_data) {
  return ImportDataBuilder{node_service}.Run(export_data);
}
