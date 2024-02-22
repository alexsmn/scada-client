#include "diff_data_builder.h"

#include "export/configuration/export_data.h"

#include <unordered_set>

class DiffDataBuilder {
 public:
  DiffData Run(const ExportData& old_export_data,
               const ExportData& new_export_data) && {
    for (const auto& export_node : old_export_data.nodes) {
      old_nodes_.try_emplace(export_node.node_id, &export_node);
      deleted_node_ids_.emplace(export_node.node_id);
    }

    for (const auto& export_node : new_export_data.nodes) {
      ProcessNode(export_node);
      deleted_node_ids_.erase(export_node.node_id);
    }

    for (const auto& deleted_node_id : deleted_node_ids_) {
      diff_data_.delete_nodes.emplace_back(deleted_node_id);
    }

    return std::move(diff_data_);
  }

 private:
  void ProcessNode(const ExportData::Node& new_node) {
    const auto* old_node = FindOldNode(new_node.node_id);

    scada::NodeAttributes attrs;
    if (!old_node || old_node->display_name != new_node.display_name) {
      attrs.set_display_name(new_node.display_name);
    }

    // Props & refs.
    scada::NodeProperties props;
    std::vector<DiffData::Reference> refs;

    for (const auto& export_value : new_node.property_values) {
      if (export_value.reference) {
        auto old_target_id = old_node->GetTargetId(export_value.prop_decl_id);
        if (old_target_id != export_value.target_id) {
          refs.emplace_back(export_value.prop_decl_id, old_target_id,
                            export_value.target_id);
        }
      } else {
        if (old_node) {
          auto value = old_node->GetPropValue(export_value.prop_decl_id);
          if (value == export_value.value) {
            continue;
          }
        }

        props.emplace_back(export_value.prop_decl_id, export_value.value);
      }
    }

    if (old_node) {
      if (!attrs.empty() || !props.empty() || !refs.empty()) {
        diff_data_.modify_nodes.emplace_back(
            new_node.node_id, new_node.type_id, new_node.parent_id,
            std::move(attrs), std::move(props), std::move(refs));
      }
    } else {
      diff_data_.create_nodes.emplace_back(new_node.node_id, new_node.type_id,
                                           new_node.parent_id, std::move(attrs),
                                           std::move(props), std::move(refs));
    }
  }

  const ExportData::Node* FindOldNode(const scada::NodeId& node_id) const {
    auto it = old_nodes_.find(node_id);
    return it != old_nodes_.end() ? it->second : nullptr;
  }

  std::unordered_map<scada::NodeId, const ExportData::Node*> old_nodes_;

  std::unordered_set<scada::NodeId> deleted_node_ids_;

  DiffData diff_data_;
};

DiffData BuildImportData(const ExportData& old_export_data,
                         const ExportData& new_export_data) {
  return DiffDataBuilder{}.Run(old_export_data, new_export_data);
}
