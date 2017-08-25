#include "base/pickle.h"
#include "views/item_drag_data.h"

void ItemDragData::Save(ui::OSExchangeData& data) const {
  auto node_string = item_id_.ToString();
  base::Pickle pickle;
  pickle.WriteString(node_string);
  data.SetPickledData(GetCustomFormat(), pickle);
}

bool ItemDragData::Load(const ui::OSExchangeData& data) {
  base::Pickle pickle;
  if (!data.GetPickledData(GetCustomFormat(), pickle))
    return false;

  base::StringPiece node_string;
  base::PickleIterator it(pickle);
  if (!it.ReadStringPiece(&node_string))
    return false;

  item_id_ = scada::NodeId::FromString(node_string);
  return !item_id_.is_null();
}

// static
ui::OSExchangeData::CustomFormat ItemDragData::GetCustomFormat() {
  static const ui::OSExchangeData::CustomFormat kFormat =
      ui::OSExchangeData::RegisterCustomFormat("telecontrol/scada/item");
  return kFormat;
}
