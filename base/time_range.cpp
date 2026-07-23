#include "base/time_range.h"

#include "base/check.h"
#include "base/struct_writer.h"
#include "base/time/calendar.h"

#include <string_view>

namespace {

const std::string_view kTimeRangeStrings[] = {
    "Custom", "Interval", "Day", "Week", "Month",
};

static_assert(std::size(kTimeRangeStrings) ==
              static_cast<size_t>(TimeRange::Type::Count));

inline bool CompareBounds(scada::base::Time a,
                          scada::base::Time b,
                          bool dates) {
  if (dates)
    return scada::base::LocalMidnight(a) == scada::base::LocalMidnight(b);
  else
    return a == b;
}

// WARNING: The function operates with UTC time unlikely to `TimeRange` types
// using local time.
scada::base::Time AlignTime(scada::base::Time time,
                            scada::base::TimeDelta interval) {
  return time - (time - scada::base::Time{}) % interval;
}

}  // namespace

scada::DateTimeRange ToDateTimeRangeWithOpenRange(const TimeRange& time_range,
                                                  scada::base::Time now) {
  auto result = ToDateTimeRange(time_range, now);

  if (scada::base::IsNull(time_range.end) || time_range.end == scada::base::kMaxTime) {
    result.second = scada::base::kMaxTime;
  }

  return result;
}

scada::DateTimeRange ToDateTimeRange(const TimeRange& time_range,
                                     scada::base::Time now) {
  scada::base::Time from, to;

  switch (time_range.type) {
    case TimeRange::Type::Day:
      from = scada::base::LocalMidnight(now);
      break;

    case TimeRange::Type::Week: {
      scada::base::Time cur = scada::base::LocalMidnight(now);
      scada::base::Exploded ts = scada::base::LocalExplode(cur);
      // We need to start day of week from Monday instead of Sunday.
      unsigned day_of_week = (ts.day_of_week + 6) % 7;
      from = cur - std::chrono::days(day_of_week);
      break;
    }

    case TimeRange::Type::Month: {
      scada::base::Exploded ts =
          scada::base::LocalExplode(scada::base::LocalMidnight(now));
      ts.day_of_month = 1;
      from = scada::base::FromLocalExploded(ts).value_or(from);
      break;
    }

    case TimeRange::Type::Interval:
      from = AlignTime(now, time_range.interval);
      break;

    case TimeRange::Type::Custom:
      from = time_range.start;
      break;

    default:
      // Unknown type: time ranges can be restored from profile data
      // (ParseTimeRangeType yields Count for unrecognized strings); fall
      // back via the clamp below.
      break;
  }

  to = time_range.end;
  if (scada::base::IsNull(to))
    to = now;

  if (time_range.dates) {
    from = scada::base::LocalMidnight(from);
    to = scada::base::LocalMidnight(to) + std::chrono::days(1);
  }

  if (scada::base::IsNull(from) || from >= to) {
    to = now;
    from = to - std::chrono::hours(1);
  }

  scada::base::Check(!scada::base::IsNull(from));
  scada::base::Check(!scada::base::IsNull(to));
  scada::base::Check(from <= to);

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
