#pragma once

#include "base/check.h"
#include "base/ostream_formatter.h"
#include "base/time/time.h"
#include "scada/date_time_range.h"

// TODO: Rename.
struct TimeRange {
  // `Day` might be merged into `Interval`, but it's kept for backward
  // compatibility.
  enum class Type { Custom, Interval, Day, Week, Month, Count };

  TimeRange() {}

  TimeRange(Type type) : type{type} {
    scada::base::Check(type != Type::Custom && type != Type::Interval &&
                       type != Type::Count);
  }

  TimeRange(scada::base::TimeDelta interval)
      : type{Type::Interval}, interval{interval} {
    scada::base::Check(!interval.is_zero());
  }

  TimeRange(scada::base::Time start, scada::base::Time end, bool dates = false)
      : type{Type::Custom}, start{start}, end{end}, dates{dates} {}

  bool is_interval() const { return type == Type::Interval; }
  bool is_custom() const { return type == Type::Custom; }

  bool operator==(const TimeRange& other) const = default;

  Type type = Type::Day;

  // Only when `type == Type::Custom`.
  scada::base::Time start;
  scada::base::Time end;
  bool dates = false;

  // Only when `type == Type::Interval`.
  scada::base::TimeDelta interval;
};

scada::DateTimeRange ToDateTimeRange(const TimeRange& time_range,
                                     scada::base::Time now);
scada::DateTimeRange ToDateTimeRangeWithOpenRange(const TimeRange& time_range,
                                                  scada::base::Time now);

std::string ToString(TimeRange::Type type);
std::string ToString(const TimeRange& time_range);

TimeRange::Type ParseTimeRangeType(std::string_view str);

inline std::ostream& operator<<(std::ostream& stream, TimeRange::Type type) {
  return stream << ToString(type);
}

std::ostream& operator<<(std::ostream& stream, const TimeRange& time_range);

// std::format support (used by base::AsOpt / AsList element rendering),
// delegating to the operator<< overloads above.
template <>
struct std::formatter<TimeRange::Type> : OStreamFormatter {};

template <>
struct std::formatter<TimeRange> : OStreamFormatter {};