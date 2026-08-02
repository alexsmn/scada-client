#pragma once

#include "base/any_executor.h"
#include "scada/node_id.h"

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

  // Starts the one-time browse if it has not run yet. Cheap to call repeatedly.
  void EnsurePopulated();

  // The tags collected so far (complete once the browse finishes).
  std::span<const Tag> tags() const SCADA_LIFETIME_BOUND { return *tags_; }

 private:
  AnyExecutor executor_;
  NodeService& node_service_;
  scada::NodeId root_;
  size_t max_tags_;
  bool started_ = false;
  // Shared with the browse coroutine so results survive this object's teardown.
  std::shared_ptr<std::vector<Tag>> tags_ =
      std::make_shared<std::vector<Tag>>();
};
