#include "modules/transmission/transmission_devices.h"

#include "model/devices_node_ids.h"
#include "model/scada_node_ids.h"
#include "node_service/node_awaitable.h"
#include "node_service/node_service.h"
#include "node_service/node_util.h"

scada::NodeId TransmissionItemTypeFor(const NodeRef& device) {
  // The transmission item type is named by the device type's <TransmissionItem>
  // OptionalPlaceholder, attached via the HasTransmissionItem reference (the
  // standard-modelling replacement for the old Creates edge). Query that exact
  // reference type so it resolves without fetching the reference-type hierarchy
  // (matters for a remote node service).
  for (auto type = device.type_definition(); type; type = type.supertype()) {
    for (const auto& placeholder :
         type.targets(scada::devices::id::HasTransmissionItem)) {
      NodeRef item_type = placeholder.type_definition();
      if (IsSubtypeOf(item_type, scada::devices::id::TransmissionItemType))
        return item_type.node_id();
    }
  }
  return {};
}

bool SupportsTransmission(const NodeRef& device) {
  return !TransmissionItemTypeFor(device).is_null();
}

int CountTransmissionRules(const NodeRef& device) {
  int count = 0;
  for (const NodeRef::Reference& reference :
       device.references(scada::id::Organizes)) {
    if (IsInstanceOf(reference.target,
                     scada::devices::id::TransmissionItemType))
      ++count;
  }
  return count;
}

Awaitable<std::vector<TransmissionDeviceEntry>> BrowseTransmissionDevices(
    NodeRef root) {
  std::vector<TransmissionDeviceEntry> entries;

  // Depth-first in reference order so the rail matches the hardware tree.
  // A transmission device's own children are its rules, so the walk stops
  // there; the depth bound guards against reference cycles.
  constexpr int kMaxDepth = 6;
  struct PendingNode {
    NodeRef node;
    int depth;
  };
  std::vector<PendingNode> pending{{std::move(root), 0}};

  while (!pending.empty()) {
    PendingNode item = std::move(pending.back());
    pending.pop_back();

    (void)co_await FetchNodeStatus(item.node);
    (void)co_await FetchTypeChainStatus(item.node.type_definition());

    if (item.depth > 0 && SupportsTransmission(item.node)) {
      (void)co_await FetchChildrenStatus(item.node);
      // The rule filter reads each child's type chain.
      for (const NodeRef::Reference& reference :
           item.node.references(scada::id::Organizes))
        (void)co_await FetchTypeChainStatus(reference.target.type_definition());
      entries.push_back({item.node.node_id(), GetFullDisplayName(item.node),
                         CountTransmissionRules(item.node)});
      continue;
    }

    if (item.depth >= kMaxDepth)
      continue;
    (void)co_await FetchChildrenStatus(item.node);
    const std::vector<NodeRef::Reference> children =
        item.node.references(scada::id::Organizes);
    for (auto it = children.rbegin(); it != children.rend(); ++it)
      pending.push_back({it->target, item.depth + 1});
  }

  co_return entries;
}
