#include "bulk_create/bulk_create_plan.h"

#include "common/formula_util.h"
#include "model/data_items_node_ids.h"
#include "model/devices_node_ids.h"
#include "model/node_id_util.h"
#include "scada/localized_text.h"
#include "scada/node_id.h"
#include "scada/variant.h"

#include <cstddef>
#include <utility>

namespace {

// A row the run must not create. Both conditions are the pattern engine's
// findings, and dropping them here rather than at the call site is what keeps
// the wizard's "will create N of M" footer equal to the size of the result.
bool IsSkipped(const BulkCreatePreviewRow& row) {
  return row.conflict || row.ioa_out_of_range;
}

}  // namespace

std::vector<scada::NodeState> PlanBulkCreate(
    const BulkCreatePlan& plan,
    const std::vector<BulkCreatePreviewRow>& rows) {
  std::vector<scada::NodeState> nodes;
  nodes.reserve(rows.size());

  // Indexes `plan.source_node_ids`, which is parallel to the *kept* rows rather
  // than to all of them: a selection of N sources produces N rules, and a row
  // dropped as a conflict must not consume the source the next row needs.
  std::size_t source_index = 0;

  for (const BulkCreatePreviewRow& row : rows) {
    if (IsSkipped(row))
      continue;

    scada::NodeState node{
        // The template's form is OPC UA's (`ns=2;s=...`), so it takes
        // NodeId::FromString and NOT NodeIdFromScadaString, which parses this
        // tree's own `TS.105` shorthand and would silently mangle it.
        .node_id = scada::NodeId::FromString(
            std::string{row.node_id.begin(), row.node_id.end()}),
        .type_definition_id = plan.type_definition_id,
        .parent_id = plan.parent_id,
        .attributes = {.display_name = scada::ToLocalizedText(row.name)},
    };

    switch (plan.subject) {
      case BulkCreateSubject::kDataItem: {
        // The source the item reads, as the shipped MultiCreateModel builds it:
        // the per-row path nested under the chosen device, wrapped as a
        // formula. The path is a template so it steps with the row, exactly
        // like the name.
        //
        // With no device chosen or no path given there is no source to bind,
        // and the item is created unbound rather than bound to nothing. Both
        // are reachable -- a deployment with no devices configured leaves the
        // wizard's device list empty -- and `MakeNestedNodeId` **panics** on a
        // null parent, so this cannot be left to it: the operator would crash
        // the client by pressing Create.
        const std::u16string path =
            ExpandTokens(plan.source_path_template, row.number);
        if (!plan.source_device_id.is_null() && !path.empty()) {
          const scada::NodeId source_id = scada::MakeNestedNodeId(
              plan.source_device_id, std::string{path.begin(), path.end()});
          node.set_property(scada::data_items::id::DataItemType_Input1,
                            scada::Variant{MakeNodeIdFormula(source_id)});
        }
        break;
      }
      case BulkCreateSubject::kTransmissionItem: {
        // A rule forwards an existing node, so its source is selected rather
        // than named. Running out of sources ends the run rather than creating
        // rules that forward nothing.
        if (source_index >= plan.source_node_ids.size())
          return nodes;
        node.set_property(scada::devices::id::TransmissionItemType_SourceNode,
                          scada::Variant{plan.source_node_ids[source_index]});
        ++source_index;
        node.set_property(scada::devices::id::TransmissionItemType_Address,
                          scada::Variant{row.ioa});
        break;
      }
    }

    nodes.push_back(std::move(node));
  }

  return nodes;
}
