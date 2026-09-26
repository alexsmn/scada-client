#include "ui/dragdrop/item_drag_data.h"

#include "remote/protocol_utils.h"

ItemDragData::ItemDragData(scada::NodeId item_id) {
  node_ids_.emplace_back(std::move(item_id));
}

std::string ItemDragData::Serialize() const {
  protocol::DragNodes message;
  for (const scada::NodeId& node_id : node_ids_)
    Convert(node_id, *message.add_node_id());
  return message.SerializeAsString();
}

bool ItemDragData::Deserialize(std::string_view bytes) {
  protocol::DragNodes message;
  if (!message.ParseFromArray(bytes.data(), static_cast<int>(bytes.size())))
    return false;

  // Drag-drop payload is external data: an empty message parses cleanly, and
  // neither an empty drag nor a null id is something a drag can carry.
  if (message.node_id().empty())
    return false;

  std::vector<scada::NodeId> node_ids;
  node_ids.reserve(message.node_id_size());
  for (const protocol::NodeId& source : message.node_id()) {
    scada::NodeId node_id;
    Convert(source, node_id);
    if (node_id.is_null())
      return false;
    node_ids.emplace_back(std::move(node_id));
  }

  node_ids_ = std::move(node_ids);
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
