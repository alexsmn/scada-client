#pragma once

#include "base/any_executor.h"
#include "base/awaitable.h"
#include "parameter_form/address_map_row.h"

#include <string>
#include <vector>

namespace scada {
class NodeId;
}
class NodeRef;

// The signal-type tag (TS for a discrete item, TI for an analog item) for a
// data-item type definition; empty for types without a simple tag. Pure, so it
// is unit-testable without a node service.
std::u16string SignalTypeTag(const scada::NodeId& type_definition_id);

// Browses `device`'s transmission items and builds its address-map rows — the
// source data item's name and type tag, the source address (IOA), and the
// source node id — for config-workbench's address-map preview. Fetches what it
// reads, so it is safe to call on a freshly-opened device. Returns an empty
// list for a device without transmission items.
Awaitable<std::vector<AddressMapRow>> BuildDeviceAddressMap(AnyExecutor executor,
                                                            NodeRef device);
