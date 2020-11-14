#pragma once

#include "base/time/time.h"

#include <cassert>

struct TimeRange {
  enum class Type { Custom, Day, Week, Month, Count };

  TimeRange() {}

  TimeRange(Type type) : type{type} { assert(type != Type::Custom); }

  TimeRange(base::Time start, base::Time end, bool dates = false)
      : start{start}, end{end}, dates{dates}, type{Type::Custom} {}

  Type type = Type::Day;
  base::Time start;
  base::Time end;
  bool dates = false;
};

bool operator==(const TimeRange& a, const TimeRange& b);

std::pair<base::Time, base::Time> GetTimeRangeBounds(
    const TimeRange& time_range);

std::string ToString(TimeRange::Type type);
std::string ToString(const TimeRange& time_range);

TimeRange::Type ParseTimeRangeType(std::string_view str);
