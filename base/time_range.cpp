#include "base/time_range.h"

#include "base/struct_writer.h"

#include <string_view>

namespace {

const std::string_view kTimeRangeStrings[] = {
    "Custom", "Interval", "Day", "Week", "Month",
};

static_assert(std::size(kTimeRangeStrings) ==
              static_cast<size_t>(TimeRange::Type::Count));

inline bool CompareBounds(base::Time a, base::Time b, bool dates) {
  if (dates)
    return a.LocalMidnight() == b.LocalMidnight();
  else
    return a == b;
}

// WARNING: The function operates with UTC time unlikely to `TimeRange` types
// using local time.
base::Time AlignTime(base::Time time, base::TimeDelta interval) {
  return time - (time - base::Time::UnixEpoch()) % interval;
}

}  // namespace

scada::DateTimeRange ToDateTimeRange(const TimeRange& time_range) {
  auto now = base::Time::Now();
  base::Time from, to;

  switch (time_range.type) {
    case TimeRange::Type::Day:
      from = now.LocalMidnight();
      break;

    case TimeRange::Type::Week: {
      base::Time cur = now.LocalMidnight();
      base::Time::Exploded ts = {};
      cur.LocalExplode(&ts);
      // We need to start day of week from Monday instead of Sunday.
      unsigned day_of_week = (ts.day_of_week + 6) % 7;
      from = cur - base::TimeDelta::FromDays(day_of_week);
      break;
    }

    case TimeRange::Type::Month: {
      base::Time::Exploded ts;
      now.LocalMidnight().LocalExplode(&ts);
      ts.day_of_month = 1;
      base::Time::FromLocalExploded(ts, &from);
      break;
    }

    case TimeRange::Type::Interval:
      from = AlignTime(now, time_range.interval);
      break;

    case TimeRange::Type::Custom:
      from = time_range.start;
      break;

    default:
      assert(false);
      break;
  }

  to = time_range.end;
  if (to.is_null())
    to = now;

  if (time_range.dates) {
    from = from.LocalMidnight();
    to = to.LocalMidnight() + base::TimeDelta::FromDays(1);
  }

  if (from >= to) {
    to = now;
    from = to - base::TimeDelta::FromHours(1);
  }

  assert(!from.is_null());
  assert(!to.is_null());
  assert(from <= to);

  return {from, to};
}

std::string ToString(TimeRange::Type type) {
  auto index = static_cast<size_t>(type);
  return index < static_cast<size_t>(TimeRange::Type::Count)
             ? std::string{kTimeRangeStrings[index]}
             : "Unknown";
}

std::string ToString(const TimeRange& time_range) {
  return ToString(time_range.type);
}

TimeRange::Type ParseTimeRangeType(std::string_view str) {
  auto i = std::ranges::find(kTimeRangeStrings, str);
  return i != std::end(kTimeRangeStrings)
             ? static_cast<TimeRange::Type>(i - std::begin(kTimeRangeStrings))
             : TimeRange::Type::Count;
}

std::ostream& operator<<(std::ostream& stream, const TimeRange& time_range) {
  StructWriter{stream}
      .AddField("type", time_range.type)
      .AddField("start", time_range.start)
      .AddField("end", time_range.end)
      .AddField("dates", time_range.dates)
      .AddField("interval", time_range.interval);
  return stream;
}
