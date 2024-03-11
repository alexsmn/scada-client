#include "window_definition_util.h"

#include "base/base64.h"
#include "base/range_util.h"
#include "base/strings/string_util.h"
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

WindowItem LoadWindowItem(const base::Value& item_data) {
  WindowItem item;
  item.name = std::string{GetString(item_data, "name")};
  if (auto* value = item_data.FindKey("@value")) {
    item.attributes = value->Clone();
  } else {
    item.attributes = item_data.Clone();
    item.attributes.RemoveKey("name");
  }
  return item;
}

base::Value SaveWindowItem(const WindowItem& item) {
  if (item.attributes.is_dict()) {
    assert(!item.attributes.FindKey("name"));
    assert(!item.attributes.FindKey("@value"));
    auto item_data = item.attributes.Clone();
    SetKey(item_data, "name", item.name);
    return item_data;
  } else {
    base::Value item_data{base::Value::Type::DICTIONARY};
    item_data.SetKey("name", base::Value{item.name});
    item_data.SetKey("@value", item.attributes.Clone());
    return item_data;
  }
}

}  // namespace

template <>
std::optional<base::Time> FromJson(const base::Value& value) {
  base::StringPiece str;
  if (!value.GetAsString(&str))
    return std::nullopt;

  base::Time time;
  if (!Deserialize(str, time))
    return std::nullopt;

  return time;
}

template <>
std::optional<TimeRange> FromJson(const base::Value& value) {
  auto* type_string = value.FindKeyOfType("type", base::Value::Type::STRING);
  if (type_string) {
    auto type = ParseTimeRangeType(type_string->GetString());
    if (type == TimeRange::Type::Count)
      return std::nullopt;

    if (type != TimeRange::Type::Custom)
      return type;
  }

  if (auto* interval_value = value.FindKey("interval")) {
    auto interval = FromJson<base::TimeDelta>(*interval_value);
    if (!interval.has_value() || interval->is_zero())
      return std::nullopt;

    return TimeRange{*interval};
  }

  // Custom time range.

  auto* start_value = value.FindKey("start");
  auto* end_value = value.FindKey("end");
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
std::optional<base::TimeDelta> FromJson(const base::Value& value) {
  return base::TimeDelta::FromSeconds(
             GetKey<int>(value, "seconds").value_or(0)) +
         base::TimeDelta::FromMinutes(
             GetKey<int>(value, "minutes").value_or(0)) +
         base::TimeDelta::FromHours(GetKey<int>(value, "hours").value_or(0)) +
         base::TimeDelta::FromDays(GetKey<int>(value, "days").value_or(0));
}

base::Value ToJson(base::Time time) {
  return base::Value{SerializeToString(time)};
}

base::Value ToJson(const TimeRange& time_range) {
  base::Value result{base::Value::Type::DICTIONARY};
  if (time_range.type == TimeRange::Type::Interval) {
    result.SetKey("interval", ToJson(time_range.interval));
  } else if (time_range.type == TimeRange::Type::Custom) {
    result.SetKey("start", ToJson(time_range.start));
    result.SetKey("end", ToJson(time_range.end));
    SetKey(result, "dates", time_range.dates);
  } else {
    SetKey(result, "type", ToString(time_range.type));
  }
  return result;
}

base::Value ToJson(base::TimeDelta duration) {
  base::Value result{base::Value::Type::DICTIONARY};
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
  auto trimmed_text =
      base::TrimString(text, base::kWhitespaceASCII, base::TRIM_ALL);
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
std::optional<WindowItems> FromJson(const base::Value& data) {
  if (!data.is_list())
    return std::nullopt;

  return data.GetList() | boost::adaptors::transformed(&LoadWindowItem) |
         to_vector;
}

base::Value ToJson(const WindowItems& items) {
  return base::Value{items | boost::adaptors::transformed(&SaveWindowItem) |
                     to_vector};
}

base::Value ToJson(const WindowDefinition& def) {
  const WindowInfo& window_info = def.window_info();

  base::Value win{base::Value::Type::DICTIONARY};
  SetKey(win, "type", window_info.name);
  SetKey(win, "id", def.id);
  if (!def.visible) {
    assert(window_info.flags & WIN_SING);
    SetKey(win, "visible", def.visible);
  }
  if (!def.title.empty())
    SetKey(win, "title", def.title);
  if (!def.path.empty())
    SetKey(win, "path", def.path.u16string());
  SetKey(win, "width", def.size.width);
  SetKey(win, "height", def.size.height);
  SetKey(win, "locked", def.locked);

  win.SetKey("items", ToJson(def.items));
  win.SetKey("data", def.storage.Clone());

  return win;
}

template <>
std::optional<WindowDefinition> FromJson(const base::Value& win) {
  const WindowInfo* info = FindWindowInfoByName(GetString(win, "type"));
  if (!info)
    return std::nullopt;

  WindowDefinition w{*info};
  w.id = GetInt(win, "id", 0);
  if (info->flags & WIN_SING)
    w.visible = GetBool(win, "visible", true);
  w.title = GetString16(win, "title");
  w.path = std::filesystem::path(GetString16(win, "path"));
  w.size = {GetInt(win, "width"), GetInt(win, "height")};
  if (auto* items = win.FindKey("items"))
    w.items = FromJson<WindowItems>(*items).value_or(WindowItems{});

  if (auto* data = FindDict(win, "data"))
    w.storage = data->Clone();

  return w;
}

base::Value ToJson(const scada::NodeId& node_id) {
  return node_id.is_null() ? base::Value{}
                           : base::Value{NodeIdToScadaString(node_id)};
}

template <>
std::optional<scada::NodeId> FromJson(const base::Value& json) {
  if (!json.is_string())
    return std::nullopt;
  auto node_id = NodeIdFromScadaString(json.GetString());
  return node_id.is_null() ? std::optional<scada::NodeId>{}
                           : std::optional<scada::NodeId>{std::move(node_id)};
}
