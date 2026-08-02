#include "node_properties/device_limits.h"

#include "base/utf_convert.h"
#include "model/data_items_node_ids.h"
#include "node_service/node_ref.h"
#include "node_service/node_util.h"
#include "scada/localized_text.h"
#include "scada/standard_node_ids.h"
#include "scada/variant.h"

#include <cstdio>
#include <string>
#include <utility>

namespace {

// Formats a limit value compactly: no trailing zeros, integer when integral.
std::u16string FormatLimitValue(double value) {
  char buffer[32];
  std::snprintf(buffer, sizeof(buffer), "%.3f", value);
  std::string text = buffer;
  if (text.find('.') != std::string::npos) {
    text.erase(text.find_last_not_of('0') + 1);
    if (!text.empty() && text.back() == '.')
      text.pop_back();
  }
  return UtfConvert<char16_t>(text);
}

// Reads one analog-item limit property; empty when the item has no such band.
Awaitable<std::u16string> FetchLimit(NodeRef item,
                                     const scada::NodeId& limit_id) {
  NodeRef property = item[limit_id];
  if (!property)
    co_return std::u16string{};
  co_await property.Fetch(NodeFetchStatus::NodeOnly);
  double value = 0;
  if (!property.value().get(value))
    co_return std::u16string{};
  co_return FormatLimitValue(value);
}

}  // namespace

Awaitable<std::vector<LimitRow>> BuildDeviceLimits(AnyExecutor executor,
                                                   NodeRef device) {
  std::vector<LimitRow> rows;

  co_await device.Fetch(NodeFetchStatus::NodeAndChildren);
  for (NodeRef& child : device.targets(scada::id::Organizes)) {
    co_await child.Fetch(NodeFetchStatus::NodeAndChildren);

    // Fetch the type chain: needed both for IsInstanceOf and for
    // operator[](limit_id) to resolve the band aggregate declarations.
    for (NodeRef type = child.type_definition(); type;) {
      co_await type.Fetch(NodeFetchStatus::NodeAndChildren);
      type = type.supertype();
    }

    if (!IsInstanceOf(child, scada::data_items::id::AnalogItemType))
      continue;

    LimitRow row;
    row.signal = ToString16(child.display_name());
    row.lolo = co_await FetchLimit(
        child, scada::data_items::id::AnalogItemType_LimitLoLo);
    row.lo = co_await FetchLimit(child,
                                 scada::data_items::id::AnalogItemType_LimitLo);
    row.hi = co_await FetchLimit(child,
                                 scada::data_items::id::AnalogItemType_LimitHi);
    row.hihi = co_await FetchLimit(
        child, scada::data_items::id::AnalogItemType_LimitHiHi);
    rows.push_back(std::move(row));
  }

  co_return rows;
}
