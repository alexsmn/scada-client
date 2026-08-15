
#include "screenshot_wait.h"

#include "base/any_executor.h"
#include "base/thread_executor.h"
#include "node_service/node_awaitable.h"
#include "node_service/node_fetch_status.h"
#include "node_service/node_ref.h"
#include "node_service/node_service.h"
#include "scada/standard_node_ids.h"
#include "timed_data/timed_data_service_impl.h"

#include <gtest/gtest.h>

#include <QElapsedTimer>

#include <vector>

namespace scada::screenshot_generator {

namespace {

constexpr int kPendingDataTimeoutMs = 30'000;

}  // namespace

bool WaitForPendingNodeLoads(NodeService& node_service) {
  try {
    WaitForAwaitable(ThreadExecutor{}, WaitForPendingNodes(node_service));
    return true;
  } catch (...) {
    ADD_FAILURE() << "NodeService pending-node wait failed";
    return true;
  }
}

bool WaitForPendingData(NodeService& node_service,
                        TimedDataService& timed_data_service) {
  // Only the real service tracks outstanding history; a fake or mock backend
  // has nothing in flight, so the node wait alone is the whole answer.
  auto* service = dynamic_cast<TimedDataServiceImpl*>(&timed_data_service);

  QElapsedTimer elapsed;
  elapsed.start();

  for (;;) {
    if (!WaitForPendingNodeLoads(node_service))
      return false;

    if (!service || !service->HasPendingHistory())
      return true;

    if (elapsed.hasExpired(kPendingDataTimeoutMs)) {
      ADD_FAILURE() << "Timed data still waiting on history after "
                    << kPendingDataTimeoutMs / 1000
                    << "s; the capture would show incomplete trends";
      return false;
    }

    // History arrives on the executor + Qt event loop, so give it a turn
    // before re-testing. A node fetch may also have been queued behind it,
    // which is why the loop goes back through WaitForPendingNodeLoads.
    PumpEventLoopFor(std::chrono::milliseconds{50});
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
  std::vector<NodeRef> types;
  for (const scada::NodeId& id : node_ids) {
    if (id.is_null())
      continue;
    NodeRef node = node_service.GetNode(id);
    if (!node)
      continue;
    if (NodeRef type = node.type_definition())
      types.push_back(std::move(type));
    for (const NodeRef& child : node.targets(scada::id::HierarchicalReferences))
      child.StartFetch(NodeFetchStatus::NodeOnly);
  }

  // Wave 3: the rest of the type's supertype chain, one level per round.
  // NodeModelImpl::GetAggregateDeclaration walks that chain looking for the
  // declaration, and an unfetched type reports no aggregates at all — so a
  // property declared on a base type resolves to nothing while its siblings
  // on the leaf type resolve fine. That is how TIT.212's
  // DataItemType_OutputCondition read back empty, leaving the control dialog
  // without its condition row, while the AnalogItemType limit bands beside it
  // read correctly. A round per level because `supertype()` reads the id off
  // the type's own state: each level must be fetched before the next can be
  // named.
  constexpr int kMaxTypeDepth = 16;
  for (int depth = 0; depth < kMaxTypeDepth && !types.empty(); ++depth) {
    for (const NodeRef& type : types)
      type.StartFetch(NodeFetchStatus::NodeAndChildren);
    if (!WaitForPendingNodeLoads(node_service))
      return false;

    std::vector<NodeRef> supertypes;
    for (const NodeRef& type : types) {
      if (NodeRef supertype = type.supertype())
        supertypes.push_back(std::move(supertype));
    }
    types = std::move(supertypes);
  }
  return WaitForPendingNodeLoads(node_service);
}

}  // namespace scada::screenshot_generator
