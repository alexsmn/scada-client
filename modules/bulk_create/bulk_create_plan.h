#pragma once

#include "bulk_create/bulk_create_pattern.h"
#include "common/node_state.h"
#include "scada/node_id.h"

#include <string>
#include <vector>

// Turns the wizard's expanded preview rows into the nodes to create.
//
// This is the half `bulk_create_pattern.h` deliberately does not do: that file
// is the pure naming/addressing engine and knows nothing of the address space,
// while this one names node types and properties. It is still free of Qt and of
// the node service — it produces `scada::NodeState` values and posts nothing —
// so the mapping from "23 preview rows" to "23 nodes with these types, parents
// and properties" is unit-testable without a server.

// Everything a run needs beyond the pattern to say what each node *is*. The
// subject decides which half is read; the other is ignored, so a stale value
// left over from the operator switching subject cannot reach the created node.
struct BulkCreatePlan {
  BulkCreateSubject subject = BulkCreateSubject::kDataItem;

  // Where the created nodes go. For a data item this is the configuration
  // folder the wizard was opened on; for a transmission item it is the
  // destination device, which is what `TransmissionModel` parents rules onto.
  scada::NodeId parent_id;

  // What they are. A data item is `DiscreteItemType` or `AnalogItemType`; a
  // transmission item is the destination's *protocol subtype*, which the
  // caller resolves (`TransmissionItemTypeFor`) because it depends on the
  // destination device rather than on anything here.
  scada::NodeId type_definition_id;

  // Data item only — the source each item reads. `source_path_template` is
  // expanded per row with the same tokens as the name, and nested under
  // `source_device_id` to build the `Input1` formula, exactly as the shipped
  // `MultiCreateModel::Run` does.
  scada::NodeId source_device_id;
  std::u16string source_path_template;

  // Transmission item only — the node each rule forwards, in row order. Bulk
  // rule creation is driven by a *selection* of sources rather than by a
  // template, because a rule's source is an existing node rather than a name
  // to invent; that is the shape `TransmissionModel::AddContainedItem` already
  // has, one source at a time. A row with no matching entry is skipped.
  std::vector<scada::NodeId> source_node_ids;
};

// Expands `plan` over `rows` into the nodes to create, in row order.
//
// **Rows that must not be created are dropped here**, and this is the only
// place that decides it: a row whose NodeId already exists (`conflict`) or
// whose address ran past the address type (`ioa_out_of_range`). The wizard's
// "will create 23 of 24" footer is therefore the size of this result, not a
// separate count that could disagree with it.
//
// Each produced node carries the row's expanded NodeId as its **requested**
// id. That is honoured end to end — `scada::AddNodesItem::requested_id` is
// carried on the wire and used by the configuration node manager when the
// store allocates none — and it is what makes the preview truthful: the ids
// the grid shows are the ids the nodes get, and the conflict check is against
// the same namespace they land in.
std::vector<scada::NodeState> PlanBulkCreate(
    const BulkCreatePlan& plan,
    const std::vector<BulkCreatePreviewRow>& rows);
