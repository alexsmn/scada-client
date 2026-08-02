#include "main_window/tag_search_index.h"

#include "base/awaitable.h"
#include "model/data_items_node_ids.h"
#include "node_service/node_fetch_status.h"
#include "node_service/node_ref.h"
#include "node_service/node_service.h"
#include "node_service/node_util.h"
#include "scada/standard_node_ids.h"

#include <set>

namespace {

// Depth-first browse of the Organizes hierarchy under `node_id`, appending
// DataItemType leaves to `out` until it holds `max_tags`. `visited` guards
// against reference cycles. `out`/`visited` are shared so the walk stays valid
// if the owning index is torn down mid-browse.
Awaitable<void> BrowseNodeAsync(
    NodeService& node_service,
    scada::NodeId node_id,
    std::shared_ptr<std::vector<TagSearchIndex::Tag>> out,
    std::shared_ptr<std::set<scada::NodeId>> visited,
    size_t max_tags) {
  if (out->size() >= max_tags)
    co_return;
  if (!visited->insert(node_id).second)
    co_return;

  co_await node_service.Fetch(node_id, NodeFetchStatus::NodeAndChildren);

  for (NodeRef& child : node_service.GetTargets(node_id, scada::id::Organizes,
                                                /*forward=*/true)) {
    if (out->size() >= max_tags)
      co_return;
    co_await child.Fetch(NodeFetchStatus::NodeOnly);
    if (IsInstanceOf(child, scada::data_items::id::DataItemType)) {
      out->push_back({child.node_id(), GetFullDisplayName(child)});
    } else {
      co_await BrowseNodeAsync(node_service, child.node_id(), out, visited,
                               max_tags);
    }
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
            co_await BrowseNodeAsync(node_service, root, out, visited,
                                     max_tags);
          });
}
