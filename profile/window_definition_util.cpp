#include "window_definition_util.h"

#include "base/base64.h"
#include "base/range_util.h"
#include <boost/algorithm/string/trim.hpp>
#include "base/time_range.h"
#include "base/time_utils.h"
#include "base/value_util.h"
#include "controller/window_info.h"
#include "model/node_id_util.h"
#include "profile/window_definition.h"
#include "scada/node_id.h"

#include <boost/range/adaptor/transformed.hpp>
#include <optional>
#include <string_view>

namespace {

WindowItem LoadWindowItem(const boost::json::value& item_data) {
  WindowItem item;
  item.name = std::string{GetString(item_data, "name")};
  if (auto* value = item_data.is_object() ? item_data.as_object().if_contains("@value") : nullptr) {
    item.attributes = *value;
  } else {
    item.attributes = item_data;
    if (item.attributes.is_object())
      item.attributes.as_object().erase("name");
  }
  return item;
}

boost::json::value SaveWindowItem(const WindowItem& item) {
  if (item.attributes.is_object()) {
    assert(!item.attributes.as_object().if_contains("name"));
    assert(!item.attributes.as_object().if_contains("@value"));
    auto item_data = item.attributes;
    SetKey(item_data, "name", item.name);
    return item_data;
  } else {
    boost::json::value item_data{boost::json::object{}};
    item_data.as_object()["name"] = item.name;
    item_data.as_object()["@value"] = item.attributes;
    return item_data;
  }
}

}  // namespace

template <>
std::optional<base::Time> FromJson(const boost::json::value& value) {
  if (!value.is_string())
    return std::nullopt;

  base::Time time;
  if (!Deserialize(std::string_view{value.as_string()}, time))
    return std::nullopt;

  return time;
}

template <>
std::optional<TimeRange> FromJson(const boost::json::value& value) {
  if (!value.is_object())
    return std::nullopt;

  auto type_str = GetString(value, "type");
  if (!type_str.empty()) {
    auto type = ParseTimeRangeType(type_str);
    if (type == TimeRange::Type::Count)
      return std::nullopt;

    if (type != TimeRange::Type::Custom)
      return type;
  }

  if (auto* interval_value = value.as_object().if_contains("interval")) {
    auto interval = FromJson<base::TimeDelta>(*interval_value);
    if (!interval.has_value() || interval->is_zero())
      return std::nullopt;

    return TimeRange{*interval};
  }

  // Custom time range.

  auto* start_value = value.as_object().if_contains("start");
  auto* end_value = value.as_object().if_contains("end");
  if (!start_value || !end_value)
    return std::nullopt;

  auto start = FromJson<base::Time>(*start_value);
  auto end = FromJson<base::Time>(*end_value);
  if (!start.has_value() || !end.has_value())
    return std::nullopt;
  if (start->is_null())
    return std::nullopt;

  return TimeRange{*start, *end};
}

template <>
std::optional<base::TimeDelta> FromJson(const boost::json::value& value) {
  return base::TimeDelta::FromSeconds(
             GetKey<int>(value, "seconds").value_or(0)) +
         base::TimeDelta::FromMinutes(
             GetKey<int>(value, "minutes").value_or(0)) +
         base::TimeDelta::FromHours(GetKey<int>(value, "hours").value_or(0)) +
         base::TimeDelta::FromDays(GetKey<int>(value, "days").value_or(0));
}

boost::json::value ToJson(std::string_view str) {
  return boost::json::value{std::string{str}};
}

boost::json::value ToJson(base::Time time) {
  return boost::json::value{SerializeToString(time)};
}

boost::json::value ToJson(const TimeRange& time_range) {
  boost::json::value result{boost::json::object{}};
  if (time_range.type == TimeRange::Type::Interval) {
    result.as_object()["interval"] = ToJson(time_range.interval);
  } else if (time_range.type == TimeRange::Type::Custom) {
    result.as_object()["start"] = ToJson(time_range.start);
    result.as_object()["end"] = ToJson(time_range.end);
    SetKey(result, "dates", time_range.dates);
  } else {
    SetKey(result, "type", ToString(time_range.type));
  }
  return result;
}

boost::json::value ToJson(base::TimeDelta duration) {
  boost::json::value result{boost::json::object{}};
  auto value = duration.InSeconds();
  if (auto seconds = value % 60)
    SetKey(result, "seconds", static_cast<int>(seconds));
  value /= 60;
  if (auto minutes = value % 60)
    SetKey(result, "minutes", static_cast<int>(minutes));
  value /= 60;
  if (auto hours = value % 24)
    SetKey(result, "hours", static_cast<int>(hours));
  value /= 24;
  if (value)
    SetKey(result, "days", static_cast<int>(value));
  return result;
}

std::string SaveBlob(std::string_view blob) {
  std::string text;
  base::Base64Encode(blob, &text);
  return text;
}

std::string RestoreBlob(std::string_view text) {
  auto trimmed_text = boost::trim_copy(std::string{text});
  std::string blob;
  base::Base64Decode(trimmed_text, &blob);
  return blob;
}

void SaveTimeRange(WindowDefinition& definition, const TimeRange& time_range) {
  definition.AddItem("TimeRange").attributes = ToJson(time_range);
}

std::optional<TimeRange> RestoreTimeRange(const WindowDefinition& definition) {
  auto* item = definition.FindItem("TimeRange");
  if (!item)
    return std::nullopt;

  return FromJson<TimeRange>(item->attributes);
}

template <>
std::optional<WindowItems> FromJson(const boost::json::value& data) {
  if (!data.is_array())
    return std::nullopt;

  return data.as_array() | boost::adaptors::transformed(&LoadWindowItem) |
         to_vector;
}

boost::json::value ToJson(const WindowItems& items) {
  boost::json::array arr;
  arr.reserve(items.size());
  for (const auto& item : items)
    arr.emplace_back(SaveWindowItem(item));
  return boost::json::value{std::move(arr)};
}

boost::json::value ToJson(const WindowDefinition& def) {
  boost::json::value win{boost::json::object{}};
  SetKey(win, "type", def.type);
  SetKey(win, "id", def.id);

  if (!def.visible) {
    SetKey(win, "visible", def.visible);
  }

  if (!def.title.empty())
    SetKey(win, "title", def.title);

  if (!def.path.empty()) {
    SetKey(win, "path", def.path.u16string());
  }

  SetKey(win, "width", def.size.width);
  SetKey(win, "height", def.size.height);
  SetKey(win, "locked", def.locked);

  win.as_object()["items"] = ToJson(def.items);
  win.as_object()["data"] = def.storage;

  return win;
}

template <>
std::optional<WindowDefinition> FromJson(const boost::json::value& json) {
  WindowDefinition w;
  w.id = GetInt(json, "id", 0);
  w.type = GetString(json, "type");
  w.visible = GetBool(json, "visible", true);
  w.title = GetString16(json, "title");
  w.path = std::filesystem::path(GetString16(json, "path"));
  w.size = {GetInt(json, "width"), GetInt(json, "height")};

  if (auto* items = json.is_object() ? json.as_object().if_contains("items") : nullptr) {
    w.items = FromJson<WindowItems>(*items).value_or(WindowItems{});
  }

  if (auto* data = FindDict(json, "data")) {
    w.storage = *data;
  }

  return w;
}

boost::json::value ToJson(const scada::NodeId& node_id) {
  return node_id.is_null() ? boost::json::value{}
                           : boost::json::value{NodeIdToScadaString(node_id)};
}

template <>
std::optional<scada::NodeId> FromJson(const boost::json::value& json) {
  if (!json.is_string())
    return std::nullopt;
  auto node_id = NodeIdFromScadaString(std::string_view{json.as_string()});
  return node_id.is_null() ? std::optional<scada::NodeId>{}
                           : std::optional<scada::NodeId>{std::move(node_id)};
}
