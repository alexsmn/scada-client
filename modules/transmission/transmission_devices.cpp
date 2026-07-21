#include "modules/transmission/transmission_devices.h"

#include "model/devices_node_ids.h"
#include "model/scada_node_ids.h"
#include "node_service/node_awaitable.h"
#include "node_service/node_service.h"
#include "node_service/node_util.h"

#include <array>

namespace {

// The containment shapes a device type's <TransmissionItem> placeholder may
// attach through. The production nodeset uses the dedicated
// HasTransmissionItem reference — queried as that exact type so it resolves
// without fetching the reference-type hierarchy (matters for a remote node
// service). The shared test address space parents its placeholders via plain
// Organizes instead (the way config instances are parented); the item-type
// subtype filter keeps ordinary organized children out.
constexpr std::array<scada::NodeId, 2> kPlaceholderReferenceTypes{
    scada::devices::id::HasTransmissionItem,
    scada::NodeId{scada::id::Organizes}};

}  // namespace

scada::NodeId TransmissionItemTypeFor(const NodeRef& device) {
  // The transmission item type is named by the device type's <TransmissionItem>
  // OptionalPlaceholder (the standard-modelling replacement for the old
  // Creates edge); see kPlaceholderReferenceTypes for the attachment shapes.
  for (auto type = device.type_definition(); type; type = type.supertype()) {
    for (const scada::NodeId& reference_type_id : kPlaceholderReferenceTypes) {
      for (const auto& placeholder : type.targets(reference_type_id)) {
        NodeRef item_type = placeholder.type_definition();
        if (IsSubtypeOf(item_type, scada::devices::id::TransmissionItemType))
          return item_type.node_id();
      }
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

    // SupportsTransmission walks fetched state only: resolve each placeholder
    // candidate's item-type chain first, or the subtype filter silently fails
    // on whichever nodes no other surface happened to fetch.
    for (auto type = item.node.type_definition(); type;
         type = type.supertype()) {
      for (const scada::NodeId& reference_type_id :
           kPlaceholderReferenceTypes) {
        for (const auto& placeholder : type.targets(reference_type_id)) {
          (void)co_await FetchNodeStatus(placeholder);
          (void)co_await FetchTypeChainStatus(placeholder.type_definition());
        }
      }
    }

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
