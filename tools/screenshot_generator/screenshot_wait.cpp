
#include "screenshot_wait.h"

#include "base/any_executor.h"
#include "base/thread_executor.h"
#include "model/data_items_node_ids.h"
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

  // Wave 2b: the targets of the node's linking references, and their own
  // property children. Waves 1 and 2 reach only what hangs beneath the node,
  // and a linked node is a peer rather than a child — a discrete item's
  // HasTsFormat names a TsFormat node elsewhere in the tree, whose OpenLabel
  // and CloseLabel are property children of *it*. Without this wave the
  // reference resolves but both labels read back as empty LocalizedText, so
  // the control dialog renders a two-item combo of blank rows beside a
  // current value that fell back to the built-in «On»/«Off» — a configured
  // item that looks unconfigured rather than an obviously broken capture.
  //
  // The reference types are named one by one on purpose. Asking for the
  // NonHierarchicalReferences supertype finds nothing here: NodeModelImpl
  // resolves the subtype relation statically only for namespace-0 reference
  // types, and falls back to an address-space walk that yields false while the
  // custom type's own node is unfetched — which it is, at this point in the
  // run. Add the reference type here when a capture needs another link
  // followed.
  static constexpr scada::NodeId kLinkedReferenceTypes[] = {
      scada::data_items::id::HasTsFormat,
  };
  std::vector<NodeRef> linked;
  for (const scada::NodeId& id : node_ids) {
    if (id.is_null())
      continue;
    NodeRef node = node_service.GetNode(id);
    if (!node)
      continue;
    for (const scada::NodeId& reference_type : kLinkedReferenceTypes) {
      if (NodeRef target = node.target(reference_type))
        linked.push_back(std::move(target));
    }
  }
  if (!linked.empty()) {
    for (const NodeRef& target : linked)
      target.StartFetch(NodeFetchStatus::NodeAndChildren);
    if (!WaitForPendingNodeLoads(node_service))
      return false;
    for (const NodeRef& target : linked) {
      for (const NodeRef& child :
           target.targets(scada::id::HierarchicalReferences))
        child.StartFetch(NodeFetchStatus::NodeOnly);
      // A linked node's properties resolve through its own type's aggregate
      // declarations exactly as the subject node's do, so its type joins the
      // supertype walk below. Fetching the node and its children is not
      // enough on its own: `format[TsFormatType_OpenLabel]` asks TsFormatType
      // for the declaration, and an unfetched type reports no aggregates.
      if (NodeRef type = target.type_definition())
        types.push_back(std::move(type));
    }
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
