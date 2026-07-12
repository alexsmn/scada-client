#include "screenshot_generator_ns_compat.h"

#include "screenshot_wait.h"

#include "base/any_executor.h"
#include "base/thread_executor.h"
#include "node_service/node_awaitable.h"
#include "node_service/node_fetch_status.h"
#include "node_service/node_ref.h"
#include "node_service/node_service.h"
#include "scada/standard_node_ids.h"

#include <gtest/gtest.h>

namespace scada::screenshot_generator {

bool WaitForPendingNodeLoads(NodeService& node_service) {
  try {
    WaitForAwaitable(ThreadExecutor{}, WaitForPendingNodes(node_service));
    return true;
  } catch (...) {
    ADD_FAILURE() << "NodeService pending-node wait failed";
    return true;
  }
}

bool FetchNodesResident(NodeService& node_service,
                        std::span<const scada::NodeId> node_ids) {
  // Wave 1: each graphed instance node together with its hierarchical children
  // so the property-child references (EU range, limit bands) become known.
  bool any = false;
  for (const scada::NodeId& id : node_ids) {
    if (id.is_null())
      continue;
    NodeRef node = node_service.GetNode(id);
    if (!node)
      return false;
    node.StartFetch(NodeFetchStatus::NodeAndChildren);
    any = true;
  }
  if (!any)
    return true;
  if (!WaitForPendingNodeLoads(node_service))
    return false;

  // Wave 2: the type definition (its aggregate declarations are what let
  // `node[declaration_id]` resolve to a property child) and each property
  // child's own value.
  for (const scada::NodeId& id : node_ids) {
    if (id.is_null())
      continue;
    NodeRef node = node_service.GetNode(id);
    if (!node)
      continue;
    if (NodeRef type = node.type_definition())
      type.StartFetch(NodeFetchStatus::NodeAndChildren);
    for (const NodeRef& child : node.targets(scada::id::HierarchicalReferences))
      child.StartFetch(NodeFetchStatus::NodeOnly);
  }
  return WaitForPendingNodeLoads(node_service);
}

}  // namespace scada::screenshot_generator
