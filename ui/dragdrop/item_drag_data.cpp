#include "ui/dragdrop/item_drag_data.h"

#include "remote/protocol_utils.h"

std::string ItemDragData::Serialize() const {
  protocol::NodeId message;
  Convert(node_id_, message);
  return message.SerializeAsString();
}

bool ItemDragData::Deserialize(std::string_view bytes) {
  protocol::NodeId message;
  if (!message.ParseFromArray(bytes.data(), static_cast<int>(bytes.size())))
    return false;

  scada::NodeId node_id;
  Convert(message, node_id);
  // Drag-drop payload is external data: an empty message parses cleanly, and
  // a null id is not something a drag can carry.
  if (node_id.is_null())
    return false;

  node_id_ = std::move(node_id);
  return true;
}

void ItemDragData::Save(DragData& drag_data) const {
  std::string bytes = Serialize();
  drag_data.insert_or_assign(std::string{kMimeType},
                             std::vector<char>{bytes.begin(), bytes.end()});
}

bool ItemDragData::Load(const DragData& drag_data) {
  auto i = drag_data.find(std::string{kMimeType});
  if (i == drag_data.end())
    return false;

  const std::vector<char>& buffer = i->second;
  return Deserialize(std::string_view{buffer.data(), buffer.size()});
}
