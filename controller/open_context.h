#pragma once

#include "base/struct_writer.h"
#include "scada/node_id.h"
#include "node_service/node_ref.h"
#include "base/time_range.h"

#include <optional>
#include <ostream>
#include <vector>

// Declares the context that can be used to build a |WindowDefinition|. A
// |WindowDefinition| is view type specific, while context only defines the
// contents.
struct OpenContext {
  NodeRef node;
  std::vector<scada::NodeId> node_ids;
  std::optional<TimeRange> time_range;
};

inline std::ostream& operator<<(std::ostream& stream,
                                const OpenContext& open_context) {
  StructWriter{stream}
      .AddField("node", open_context.node.node_id())
      .AddField("node_ids", open_context.node_ids)
      .AddField("time_range", open_context.time_range);
  return stream;
}
