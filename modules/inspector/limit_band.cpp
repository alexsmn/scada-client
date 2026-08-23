#include "modules/inspector/limit_band.h"

#include "model/data_items_node_ids.h"
#include "node_service/node_fetch_status.h"

LimitBand LimitBandFor(double value, const LimitValues& limits) {
  // Most severe first, so overlapping bands report the worse breach.
  if (limits.hihi && value >= *limits.hihi)
    return LimitBand::kHiHi;
  if (limits.lolo && value <= *limits.lolo)
    return LimitBand::kLoLo;
  if (limits.hi && value >= *limits.hi)
    return LimitBand::kHi;
  if (limits.lo && value <= *limits.lo)
    return LimitBand::kLo;
  return LimitBand::kNormal;
}

Awaitable<void> FetchLimitBands(NodeRef item) {
  if (!item)
    co_return;

  co_await item.Fetch(NodeFetchStatus::NodeAndChildren);

  // The whole chain, not just the immediate type: AnalogItemType_LimitHi and
  // its siblings are declared on a supertype of most configured items, and the
  // subscript resolves an aggregate declaration only against a resident type.
  for (NodeRef type = item.type_definition(); type;) {
    co_await type.Fetch(NodeFetchStatus::NodeAndChildren);
    type = type.supertype();
  }

  // Each band node's own value. A band the item does not configure resolves to
  // a null ref and is simply skipped — that is a node with fewer bands, not a
  // failure.
  static constexpr scada::NodeId kBands[] = {
      scada::data_items::id::AnalogItemType_LimitHiHi,
      scada::data_items::id::AnalogItemType_LimitHi,
      scada::data_items::id::AnalogItemType_LimitLo,
      scada::data_items::id::AnalogItemType_LimitLoLo,
  };
  for (const scada::NodeId& band : kBands) {
    if (NodeRef property = item[band])
      co_await property.Fetch(NodeFetchStatus::NodeOnly);
  }
}
