#include "main_window/tag_search_index.h"

#include "base/awaitable.h"
#include "model/data_items_node_ids.h"
#include "node_service/node_fetch_status.h"
#include "node_service/node_ref.h"
#include "node_service/node_service.h"
#include "node_service/node_util.h"
#include "scada/standard_node_ids.h"

#include <algorithm>
#include <set>
#include <vector>

namespace {

// Concurrent subtree fetches in flight at once. Nothing below this caps
// concurrency, and an address space is wide, so issuing a whole level at once
// would put one in-flight request per container on the session.
constexpr size_t kBrowseWindow = 8;

// Breadth-first browse of the Organizes hierarchy under `root`, appending
// DataItemType leaves to `out` until it holds `max_tags`.
//
// Level by level, with each level's fetches issued together, rather than one
// subtree at a time. The walk is latency-bound: the depth-first form this
// replaced awaited a single node at a time, so it cost one round trip per
// container visited however fast the link was. Same shape as the fix in
// NodeModelImpl::OnChildrenFetched -- StartFetch is idempotent per node and
// Fetch() on an already-started node joins rather than re-requests, so the
// second pass over a window costs nothing beyond the first.
//
// The order tags are collected in changed with it, from depth-first to
// shallowest-first. That is visible only when the walk stops early at
// `max_tags`, where breadth-first is the better truncation for a palette: it
// keeps whole levels rather than one arbitrarily deep branch.
//
// `visited` guards against reference cycles. `out`/`visited` are shared so the
// walk stays valid if the owning index is torn down mid-browse.
Awaitable<void> BrowseTreeAsync(
    NodeService& node_service,
    scada::NodeId root,
    std::shared_ptr<std::vector<TagSearchIndex::Tag>> out,
    std::shared_ptr<std::set<scada::NodeId>> visited,
    size_t max_tags) {
  std::vector<scada::NodeId> level;
  if (visited->insert(root).second)
    level.push_back(std::move(root));

  while (!level.empty() && out->size() < max_tags) {
    std::vector<scada::NodeId> next;

    for (size_t begin = 0; begin < level.size() && out->size() < max_tags;
         begin += kBrowseWindow) {
      const size_t end = std::min(begin + kBrowseWindow, level.size());

      for (size_t i = begin; i < end; ++i)
        node_service.StartFetch(level[i], NodeFetchStatus::NodeAndChildren);
      for (size_t i = begin; i < end; ++i)
        co_await node_service.Fetch(level[i], NodeFetchStatus::NodeAndChildren);

      // Join the children before classifying: IsInstanceOf reads the child's
      // type definition. A parent's NodeAndChildren already pulls each child
      // NodeOnly today, so these are usually joins onto finished work -- but
      // the classification must not depend on that.
      std::vector<NodeRef> children;
      for (size_t i = begin; i < end; ++i) {
        for (NodeRef& child : node_service.GetTargets(
                 level[i], scada::id::Organizes, /*forward=*/true)) {
          children.push_back(child);
        }
      }
      for (NodeRef& child : children)
        child.StartFetch(NodeFetchStatus::NodeOnly);
      for (NodeRef& child : children)
        co_await child.Fetch(NodeFetchStatus::NodeOnly);

      for (NodeRef& child : children) {
        if (out->size() >= max_tags)
          break;
        if (IsInstanceOf(child, scada::data_items::id::DataItemType)) {
          out->push_back({child.node_id(), GetFullDisplayName(child)});
        } else if (visited->insert(child.node_id()).second) {
          next.push_back(child.node_id());
        }
      }
    }

    level = std::move(next);
  }
}

}  // namespace

TagSearchIndex::TagSearchIndex(AnyExecutor executor,
                               NodeService& node_service,
                               scada::NodeId root,
                               size_t max_tags)
    : executor_{std::move(executor)},
      node_service_{node_service},
      root_{std::move(root)},
      max_tags_{max_tags} {}

TagSearchIndex::~TagSearchIndex() = default;

void TagSearchIndex::EnsurePopulated() {
  if (started_)
    return;
  started_ = true;
  CoSpawn(executor_,
          [&node_service = node_service_, root = root_, out = tags_,
           max_tags = max_tags_]() -> Awaitable<void> {
            auto visited = std::make_shared<std::set<scada::NodeId>>();
            co_await BrowseTreeAsync(node_service, std::move(root), out,
                                     visited, max_tags);
          });
}
