#pragma once

#include "base/awaitable.h"
#include "node_service/node_ref.h"
#include "scada/node_id.h"

#include <string>
#include <vector>

// A transmission-capable device for the destination rail: identity plus its
// current rule count.
struct TransmissionDeviceEntry {
  scada::NodeId node_id;
  std::u16string name;
  int rule_count = 0;
};

// Resolves the transmission-item type named by `device`'s type chain — the
// <TransmissionItem> OptionalPlaceholder attached via HasTransmissionItem
// (the standard-modelling replacement for the old Creates edge). Null when
// the device cannot hold transmission rules. Requires the type chain fetched.
scada::NodeId TransmissionItemTypeFor(const NodeRef& device);

// True when `device` can hold transmission rules (its type chain names a
// transmission-item type). Requires the type chain fetched.
bool SupportsTransmission(const NodeRef& device);

// Counts the transmission rules under `device` — its Organizes children that
// are TransmissionItemType instances. Requires the children and their type
// chains fetched.
int CountTransmissionRules(const NodeRef& device);

// Walks the hardware tree from `root` (the Devices folder), fetching nodes as
// it goes, and returns every transmission-capable device with its rule count,
// in tree order. Feeds the destination rail.
Awaitable<std::vector<TransmissionDeviceEntry>> BrowseTransmissionDevices(
    NodeRef root);
