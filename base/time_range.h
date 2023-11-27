#pragma once

#include "base/struct_writer.h"
#include "base/time/time.h"
#include "scada/date_time_range.h"

#include <cassert>

// TODO: Rename.
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

scada::DateTimeRange ToDateTimeRange(const TimeRange& time_range);

std::string ToString(TimeRange::Type type);
std::string ToString(const TimeRange& time_range);

TimeRange::Type ParseTimeRangeType(std::string_view str);

inline std::ostream& operator<<(std::ostream& stream, TimeRange::Type type) {
  return stream << ToString(type);
}

inline std::ostream& operator<<(std::ostream& stream,
                                const TimeRange& time_range) {
  StructWriter{stream}
      .AddField("type", time_range.type)
      .AddField("start", time_range.start)
      .AddField("end", time_range.end)
      .AddField("dates", time_range.dates);
  return stream;
}
