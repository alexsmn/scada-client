#pragma once

#include <boost/json.hpp>
#include "base/relative_time_range.h"
#include "profile/window_definition.h"
#include "scada/node_id.h"

#include <optional>
#include <string_view>

template <>
std::optional<scada::Time> FromJson(const boost::json::value& value);

template <>
std::optional<scada::RelativeTimeRange> FromJson(const boost::json::value& value);

template <>
std::optional<scada::Duration> FromJson(const boost::json::value& value);

boost::json::value ToJson(std::string_view str);

boost::json::value ToJson(scada::Time time);

boost::json::value ToJson(const scada::RelativeTimeRange& time_range);

boost::json::value ToJson(scada::Duration duration);

std::string SaveBlob(std::string_view blob);

std::string RestoreBlob(std::string_view text);

void SaveTimeRange(WindowDefinition& definition, const scada::RelativeTimeRange& time_range);

std::optional<scada::RelativeTimeRange> RestoreTimeRange(const WindowDefinition& definition);

template <>
std::optional<WindowItems> FromJson(const boost::json::value& data);

boost::json::value ToJson(const WindowItems& items);

boost::json::value ToJson(const WindowDefinition& def);

template <>
std::optional<WindowDefinition> FromJson(const boost::json::value& win);

boost::json::value ToJson(const scada::NodeId& node_id);

std::optional<scada::NodeId> FromJson(const boost::json::value& json);
