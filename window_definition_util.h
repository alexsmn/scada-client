#pragma once

#include "base/base64.h"
#include "base/strings/string_piece.h"
#include "base/strings/string_util.h"
#include "base/time_utils.h"
#include "time_range.h"
#include "value_util.h"
#include "window_definition.h"

#include <optional>

template <>
inline std::optional<base::Time> FromJson(const base::Value& value) {
  base::StringPiece str;
  if (!value.GetAsString(&str))
    return std::nullopt;

  base::Time time;
  if (!Deserialize(str, time))
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

inline std::string SaveBlob(base::StringPiece blob) {
  std::string text;
  base::Base64Encode(blob, &text);
  return text;
}

inline std::string RestoreBlob(base::StringPiece text) {
  auto trimmed_text =
      base::TrimString(text, base::kWhitespaceASCII, base::TRIM_ALL);
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
