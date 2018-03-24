#include "base/pickle.h"
#include "views/item_drag_data.h"

namespace {
const uint16_t kVersion = 0;
}

void ItemDragData::Save(ui::OSExchangeData& data) const {
  base::Pickle pickle;
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

  data.SetPickledData(GetCustomFormat(), pickle);
}

bool ItemDragData::Load(const ui::OSExchangeData& data) {
  base::Pickle pickle;
  if (!data.GetPickledData(GetCustomFormat(), pickle))
    return false;

  uint16_t version = 0;
  uint16_t namespace_index = 0;
  uint16_t identifier_type = 0;

  base::PickleIterator it(pickle);
  if (!it.ReadUInt16(&version) ||
      !it.ReadUInt16(&namespace_index) ||
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

// static
ui::OSExchangeData::CustomFormat ItemDragData::GetCustomFormat() {
  static const ui::OSExchangeData::CustomFormat kFormat =
      ui::OSExchangeData::RegisterCustomFormat("telecontrol/scada/item");
  return kFormat;
}
