#pragma once

#include "base/base64.h"
#include "base/string_piece_util.h"
#include "base/strings/string_util.h"
#include "base/time_utils.h"
#include "time_range.h"
#include "value_util.h"
#include "window_definition.h"
#include "window_info.h"

#include <optional>
#include <string_view>

template <>
inline std::optional<base::Time> FromJson(const base::Value& value) {
  base::StringPiece str;
  if (!value.GetAsString(&str))
    return std::nullopt;

  base::Time time;
  if (!Deserialize(std::string_view{str.data(), str.size()}, time))
    return std::nullopt;

  return time;
}

template <>
inline std::optional<TimeRange> FromJson(const base::Value& value) {
  auto* type_string = value.FindKeyOfType("type", base::Value::Type::STRING);
  if (type_string) {
    auto type = ParseTimeRangeType(type_string->GetString());
    if (type == TimeRange::Type::Count)
      return std::nullopt;
    if (type != TimeRange::Type::Custom)
      return type;
  }

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
inline std::optional<base::TimeDelta> FromJson(const base::Value& value) {
  return base::TimeDelta::FromSeconds(
             GetKey<int>(value, "seconds").value_or(0)) +
         base::TimeDelta::FromMinutes(
             GetKey<int>(value, "minutes").value_or(0)) +
         base::TimeDelta::FromHours(GetKey<int>(value, "hours").value_or(0)) +
         base::TimeDelta::FromDays(GetKey<int>(value, "days").value_or(0));
}

inline base::Value ToJson(base::Time time) {
  return base::Value{SerializeToString(time)};
}

inline base::Value ToJson(const TimeRange& time_range) {
  base::Value result{base::Value::Type::DICTIONARY};
  if (time_range.type == TimeRange::Type::Custom) {
    result.SetKey("start", ToJson(time_range.start));
    result.SetKey("end", ToJson(time_range.end));
    SetKey(result, "dates", time_range.dates);
  } else {
    SetKey(result, "type", ToString(time_range.type));
  }
  return result;
}

inline base::Value ToJson(base::TimeDelta duration) {
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

inline std::string SaveBlob(std::string_view blob) {
  std::string text;
  base::Base64Encode(ToStringPiece(blob), &text);
  return text;
}

inline std::string RestoreBlob(std::string_view text) {
  auto trimmed_text = base::TrimString(ToStringPiece(text),
                                       base::kWhitespaceASCII, base::TRIM_ALL);
  std::string blob;
  base::Base64Decode(trimmed_text, &blob);
  return blob;
}

inline void SaveTimeRange(WindowDefinition& definition,
                          const TimeRange& time_range) {
  definition.AddItem("TimeRange").attributes = ToJson(time_range);
}

inline std::optional<TimeRange> RestoreTimeRange(
    const WindowDefinition& definition) {
  auto* item = definition.FindItem("TimeRange");
  if (!item)
    return std::nullopt;

  return FromJson<TimeRange>(item->attributes);
}

namespace {

void LoadWinItems(WindowItems& items, const base::Value& data) {
  if (!data.is_list())
    return;

  for (auto& item_data : data.GetList()) {
    auto& item = items.emplace_back();
    item.name = std::string{GetString(item_data, "name")};
    item.attributes = item_data.Clone();
    item.attributes.RemoveKey("name");
  }
}

base::Value SaveWinItems(const WindowItems& items) {
  base::Value::ListStorage list;
  list.reserve(items.size());
  for (const auto& item : items) {
    assert(item.attributes.is_dict());
    assert(!item.attributes.FindKey("name"));
    auto item_data = item.attributes.Clone();
    SetKey(item_data, "name", item.name);
    list.emplace_back(std::move(item_data));
  }
  return base::Value{std::move(list)};
}

}  // namespace

inline base::Value ToJson(const WindowDefinition& def) {
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
    SetKey(win, "path", def.path.value());
  SetKey(win, "width", def.size.width());
  SetKey(win, "height", def.size.height());
  SetKey(win, "locked", def.locked);

  win.SetKey("items", SaveWinItems(def.items));
  win.SetKey("data", def.storage.Clone());

  return win;
}

template <>
inline std::optional<WindowDefinition> FromJson(const base::Value& win) {
  UINT type = ParseWindowType(GetString(win, "type"));
  const WindowInfo* info = FindWindowInfo(type);
  if (!info)
    return std::nullopt;

  WindowDefinition w{*info};
  w.id = GetInt(win, "id", 0);
  if (info->flags & WIN_SING)
    w.visible = GetBool(win, "visible", true);
  w.title = GetString16(win, "title");
  w.path = base::FilePath(GetString16(win, "path"));
  w.size = gfx::Size(GetInt(win, "width"), GetInt(win, "height"));
  if (auto* items = win.FindKey("items"))
    LoadWinItems(w.items, *items);

  if (auto* data = GetDict(win, "data"))
    w.storage = data->Clone();

  return w;
}