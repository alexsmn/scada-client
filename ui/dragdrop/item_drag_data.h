#pragma once

#include "aui/handlers.h"
#include "base/lifetime.h"
#include "scada/node_id.h"

#include <string>
#include <string_view>

// The payload of a node dragged out of a client view: the dragged node's id,
// serialized as a `protocol::NodeId` message under `kMimeType`. The encoding
// is the protobuf one the clipboard already uses for node trees, so the two
// exchange paths share one wire vocabulary.
class ItemDragData {
 public:
  ItemDragData() = default;
  explicit ItemDragData(const scada::NodeId& item_id) : node_id_(item_id) {}

  const scada::NodeId& item_id() const SCADA_LIFETIME_BOUND { return node_id_; }

  // Encodes the node id as a serialized `protocol::NodeId`.
  std::string Serialize() const;
  // Decodes a payload produced by `Serialize`. Drag payloads come from outside
  // the process, so malformed or foreign bytes return false rather than fail.
  bool Deserialize(std::string_view bytes);

  // Adds the payload to `drag_data` under `kMimeType`.
  void Save(DragData& drag_data) const;
  // Reads the payload stored under `kMimeType`; false if absent or malformed.
  bool Load(const DragData& drag_data);

  inline static const std::string_view kMimeType =
      "application/telecontrol.scada.nodes";

 private:
  scada::NodeId node_id_;
};
