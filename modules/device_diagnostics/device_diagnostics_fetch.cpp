#include "modules/device_diagnostics/device_diagnostics_fetch.h"

#include "node_service/node_fetch_status.h"
#include "scada/standard_node_ids.h"

namespace {

// One node, its children, its whole type chain, and each child's own
// attributes. That is what `node[declaration]` and a browse-name scan of the
// children between them need: the declaration resolves against the type, and
// the scan compares browse names, which are attributes of the children
// themselves rather than of the parent that lists them.
Awaitable<void> FetchNodeAndItsChildren(NodeRef node) {
  if (!node)
    co_return;

  co_await node.Fetch(NodeFetchStatus::NodeAndChildren);

  for (NodeRef type = node.type_definition(); type;) {
    co_await type.Fetch(NodeFetchStatus::NodeAndChildren);
    type = type.supertype();
  }

  for (const NodeRef& child : node.targets(scada::id::HierarchicalReferences))
    co_await child.Fetch(NodeFetchStatus::NodeOnly);
}

}  // namespace

Awaitable<void> FetchDeviceDiagnostics(NodeRef device) {
  if (!device)
    co_return;

  co_await FetchNodeAndItsChildren(device);
  // The parent link carries the protocol's own rows — in -104 the APCI belongs
  // to the TCP connection, which several devices share — and its type is what
  // decides whether there is a link section to draw at all.
  co_await FetchNodeAndItsChildren(device.parent());
}
