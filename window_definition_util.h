#pragma once

#include "base/json.h"
#include "scada/node_id.h"
#include "time_range.h"
#include "window_definition.h"

#include <optional>
#include <string_view>

template <>
std::optional<base::Time> FromJson(const base::Value& value);

template <>
std::optional<TimeRange> FromJson(const base::Value& value);
template <>
std::optional<base::TimeDelta> FromJson(const base::Value& value);

base::Value ToJson(base::Time time);

base::Value ToJson(const TimeRange& time_range);

base::Value ToJson(base::TimeDelta duration);

std::string SaveBlob(std::string_view blob);

std::string RestoreBlob(std::string_view text);

void SaveTimeRange(WindowDefinition& definition, const TimeRange& time_range);

std::optional<TimeRange> RestoreTimeRange(const WindowDefinition& definition);

template <>
std::optional<WindowItems> FromJson(const base::Value& data);

base::Value ToJson(const WindowItems& items);

base::Value ToJson(const WindowDefinition& def);

template <>
std::optional<WindowDefinition> FromJson(const base::Value& win);

base::Value ToJson(const scada::NodeId& node_id);

std::optional<scada::NodeId> FromJson(const base::Value& json);
