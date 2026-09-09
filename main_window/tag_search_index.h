#pragma once

#include "base/any_executor.h"
#include "scada/node_id.h"

#include <boost/signals2/connection.hpp>

#include <cstddef>
#include <memory>
#include <span>
#include <string>
#include <vector>

class NodeService;

// Lazily-populated flat index of address-space data-item tags, for the command
// palette's tag search. On EnsurePopulated() it browses the address space once
// — bounded to `max_tags`, following Organizes references from `root` and
// collecting DataItemType leaves as {node_id, display name}. The browse runs on
// the executor; tags() may be partial while it is in flight and is complete
// once it finishes. Filtering is done by the caller (locally, no per-keystroke
// round-trip), so this only owns collection.
//
// The index invalidates itself: it subscribes to the node service's model and
// semantic changes, so a created, deleted or renamed node drops the collected
// tags and the next EnsurePopulated() browses again. Without that the palette
// offered deleted tags, missed created ones and showed old names for the
// window's whole lifetime. Reset() is the same thing on demand, for the
// re-login path — the window outlives a login, and the previous session's
// tags must not be offered after one.
class TagSearchIndex {
 public:
  struct Tag {
    scada::NodeId node_id;
    std::u16string name;
  };

  TagSearchIndex(AnyExecutor executor,
                 NodeService& node_service,
                 scada::NodeId root,
                 size_t max_tags = 2000);
  ~TagSearchIndex();

  TagSearchIndex(const TagSearchIndex&) = delete;
  TagSearchIndex& operator=(const TagSearchIndex&) = delete;

  // Starts the browse if it has not run since the last invalidation. Cheap to
  // call repeatedly.
  void EnsurePopulated();

  // Drops the collected tags and arms the next EnsurePopulated() to browse
  // again. A browse still in flight is abandoned rather than cancelled: it
  // keeps writing into the vector it was given, which nothing reads any more.
  void Reset();

  // The tags collected so far (complete once the browse finishes).
  std::span<const Tag> tags() const SCADA_LIFETIME_BOUND { return *tags_; }

 private:
  AnyExecutor executor_;
  NodeService& node_service_;
  scada::NodeId root_;
  size_t max_tags_;
  bool started_ = false;
  boost::signals2::scoped_connection model_changed_connection_;
  boost::signals2::scoped_connection semantic_changed_connection_;
  // Shared with the browse coroutine so results survive this object's teardown.
  std::shared_ptr<std::vector<Tag>> tags_ =
      std::make_shared<std::vector<Tag>>();
};
