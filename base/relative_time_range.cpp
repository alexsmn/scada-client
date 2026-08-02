#include "base/relative_time_range.h"

#include "base/check.h"
#include "base/struct_writer.h"
#include "base/time/calendar.h"

#include <algorithm>
#include <chrono>
#include <string_view>

namespace {

const std::string_view kTimeRangeStrings[] = {
    "Custom", "Interval", "Day", "Week", "Month",
};

static_assert(std::size(kTimeRangeStrings) ==
              static_cast<size_t>(scada::RelativeTimeRange::Type::Count));

// WARNING: The function operates with UTC time unlike `RelativeTimeRange` types
// using local time.
scada::Time AlignTime(scada::Time time, scada::Duration interval) {
  return time - (time - scada::Time{}) % interval;
}

}  // namespace

namespace scada {

TimeRange ToTimeRangeWithOpenRange(const RelativeTimeRange& range, Time now) {
  auto result = ToTimeRange(range, now);

  if (IsNull(range.end) || range.end == kMaxTime) {
    result.second = kMaxTime;
  }

  return result;
}

TimeRange ToTimeRange(const RelativeTimeRange& range, Time now) {
  Time from, to;

  switch (range.type) {
    case RelativeTimeRange::Type::Day:
      from = base::LocalMidnight(now);
      break;

    case RelativeTimeRange::Type::Week: {
      Time cur = base::LocalMidnight(now);
      base::Exploded ts = base::LocalExplode(cur);
      // We need to start day of week from Monday instead of Sunday.
      unsigned day_of_week = (ts.day_of_week + 6) % 7;
      from = cur - std::chrono::days(day_of_week);
      break;
    }

    case RelativeTimeRange::Type::Month: {
      base::Exploded ts = base::LocalExplode(base::LocalMidnight(now));
      ts.day_of_month = 1;
      from = base::FromLocalExploded(ts).value_or(from);
      break;
    }

    case RelativeTimeRange::Type::Interval:
      from = AlignTime(now, range.interval);
      break;

    case RelativeTimeRange::Type::Custom:
      from = range.start;
      break;

    default:
      // Unknown type: time ranges can be restored from profile data
      // (ParseTimeRangeType yields Count for unrecognized strings); fall
      // back via the clamp below.
      break;
  }

  to = range.end;
  if (IsNull(to))
    to = now;

  if (range.dates) {
    from = base::LocalMidnight(from);
    to = base::LocalMidnight(to) + std::chrono::days(1);
  }

  if (IsNull(from) || from >= to) {
    to = now;
    from = to - std::chrono::hours(1);
  }

  base::Check(!IsNull(from));
  base::Check(!IsNull(to));
  base::Check(from <= to);

  return {from, to};
}

RelativeTimeRange::Type ParseTimeRangeType(std::string_view str) {
  auto i = std::ranges::find(kTimeRangeStrings, str);
  return i != std::end(kTimeRangeStrings)
             ? static_cast<RelativeTimeRange::Type>(
                   i - std::begin(kTimeRangeStrings))
             : RelativeTimeRange::Type::Count;
}

std::ostream& operator<<(std::ostream& stream, RelativeTimeRange::Type type) {
  return stream << ToString(type);
}

std::ostream& operator<<(std::ostream& stream, const RelativeTimeRange& range) {
  StructWriter{stream}
      .AddField("type", range.type)
      .AddField("start", range.start)
      .AddField("end", range.end)
      .AddField("dates", range.dates)
      .AddField("interval", range.interval);
  return stream;
}

}  // namespace scada

std::string ToString(scada::RelativeTimeRange::Type type) {
  auto index = static_cast<size_t>(type);
  return index < static_cast<size_t>(scada::RelativeTimeRange::Type::Count)
             ? std::string{kTimeRangeStrings[index]}
             : "Unknown";
}

std::string ToString(const scada::RelativeTimeRange& range) {
  return ToString(range.type);
}
