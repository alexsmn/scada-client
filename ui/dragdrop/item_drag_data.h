#pragma once

#include "aui/handlers.h"
#include "base/lifetime.h"
#include "scada/node_id.h"

#include <span>
#include <string>
#include <string_view>
#include <vector>

// The payload of nodes dragged out of a client view: the dragged node ids, in
// the view's order, serialized as a `protocol::DragNodes` message under
// `kMimeType`. The encoding is the protobuf one the clipboard already uses for
// node trees, so the two exchange paths share one wire vocabulary.
class ItemDragData {
 public:
  ItemDragData() = default;
  explicit ItemDragData(scada::NodeId item_id);
  explicit ItemDragData(std::vector<scada::NodeId> item_ids)
      : node_ids_(std::move(item_ids)) {}

  // The dragged node ids; never empty after a successful `Load`.
  std::span<const scada::NodeId> item_ids() const SCADA_LIFETIME_BOUND {
    return node_ids_;
  }

  // Encodes the node ids as a serialized `protocol::DragNodes`.
  std::string Serialize() const;
  // Decodes a payload produced by `Serialize`. Drag payloads come from outside
  // the process, so malformed or foreign bytes, an empty list and a null id
  // all return false rather than fail.
  bool Deserialize(std::string_view bytes);

  // Adds the payload to `drag_data` under `kMimeType`.
  void Save(DragData& drag_data) const;
  // Reads the payload stored under `kMimeType`; false if absent or malformed.
  bool Load(const DragData& drag_data);

  inline static const std::string_view kMimeType =
      "application/telecontrol.scada.nodes";

 private:
  std::vector<scada::NodeId> node_ids_;
};
