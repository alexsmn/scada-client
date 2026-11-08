#pragma once

#include "model/devices_node_ids.h"
#include "model/namespaces.h"
#include "node_service/node_ref.h"
#include "scada/node_id.h"

// The reference a 2.5 Server links a transmission item to its source data item
// with. Version 2.6 replaced it with the SourceNode property and retired the
// constant from the model, but a 2.5.7 Server still sends it: it stores the
// link as a reference (column IecTransmitSourceNS/ID) and has no SourceNode
// property at all (read from the 2.5.7 source, scada-server-framework at
// b122fd7ad, `devices/transmission/transmission_item_props.h`). Its numeric id
// is frozen with the rest of the 2.5 model.
inline constexpr scada::NodeId kLegacyHasTransmissionSource{
    221, scada::NamespaceIndexes::SCADA};

// The data item `transmission` (a TransmissionItemType instance) retransmits,
// or a null id. Reads the SourceNode property a 2.6 Server publishes and falls
// back to the 2.5 reference, so a 2.6 Client connected to a 2.5.7 Server shows
// the sources that Server's configuration actually holds instead of blanks.
// SourceNode resolves through the type chain, so the chain must be resident;
// the reference needs the node's own references fetched.
inline scada::NodeId GetTransmissionSourceId(const NodeRef& transmission) {
  scada::NodeId source_id =
      transmission[scada::devices::id::TransmissionItemType_SourceNode]
          .value()
          .get_or(scada::NodeId{});
  if (source_id.is_null()) {
    if (NodeRef legacy = transmission.target(kLegacyHasTransmissionSource))
      source_id = legacy.node_id();
  }
  return source_id;
}
