#include "item_drag_data.h"

#include "base/pickle.h"

namespace {
const uint16_t kVersion = 0;
}

void ItemDragData::Save(aui::OSExchangeData& data) const {
  base::Pickle pickle;
  Save(pickle);
  data.SetPickledData(GetCustomFormat(), pickle);
}

bool ItemDragData::Load(const aui::OSExchangeData& data) {
  base::Pickle pickle;
  if (!data.GetPickledData(GetCustomFormat(), pickle))
    return false;

  return Load(pickle);
}

void ItemDragData::Save(base::Pickle& pickle) const {
  pickle.WriteUInt16(0);
  pickle.WriteUInt16(node_id_.namespace_index());
  pickle.WriteUInt16(static_cast<uint16_t>(node_id_.type()));
  switch (node_id_.type()) {
    case scada::NodeIdType::Numeric:
      pickle.WriteUInt32(node_id_.numeric_id());
      break;
    case scada::NodeIdType::String:
      pickle.WriteString(node_id_.string_id());
      break;
    default:
      assert(false);
      break;
  }
}

bool ItemDragData::Load(const base::Pickle& pickle) {
  uint16_t version = 0;
  uint16_t namespace_index = 0;
  uint16_t identifier_type = 0;

  base::PickleIterator it(pickle);
  if (!it.ReadUInt16(&version) || !it.ReadUInt16(&namespace_index) ||
      !it.ReadUInt16(&identifier_type))
    return false;

  if (version != 0)
    return false;

  switch (static_cast<scada::NodeIdType>(identifier_type)) {
    case scada::NodeIdType::Numeric: {
      uint32_t numeric_id = 0;
      if (!it.ReadUInt32(&numeric_id))
        return false;
      node_id_ = scada::NodeId{numeric_id, namespace_index};
      break;
    }

    case scada::NodeIdType::String: {
      std::string string_id;
      if (!it.ReadString(&string_id))
        return false;
      node_id_ = scada::NodeId{std::move(string_id), namespace_index};
      break;
    }

    default:
      assert(false);
      break;
  }

  return true;
}

void ItemDragData::Save(DragData& drag_data) const {
  base::Pickle pickle;
  Save(pickle);
  std::vector<char> buffer{
      static_cast<const char*>(pickle.data()),
      static_cast<const char*>(pickle.data()) + pickle.size()};
  drag_data.emplace(std::piecewise_construct, std::forward_as_tuple(kMimeType),
                    std::forward_as_tuple(std::move(buffer)));
}

bool ItemDragData::Load(const DragData& drag_data) {
  auto i = drag_data.find(std::string{kMimeType});
  if (i == drag_data.end())
    return false;

  const auto& buffer = i->second;
  base::Pickle pickle{buffer.data(), static_cast<int>(buffer.size())};
  return Load(pickle);
}

// static
aui::OSExchangeData::CustomFormat ItemDragData::GetCustomFormat() {
  static const aui::OSExchangeData::CustomFormat kFormat =
      aui::OSExchangeData::RegisterCustomFormat("telecontrol/scada/node");
  return kFormat;
}
