#include "base/time_range.h"

#include "base/check.h"
#include "base/struct_writer.h"

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
    return a.LocalMidnight() == b.LocalMidnight();
  else
    return a == b;
}

// WARNING: The function operates with UTC time unlikely to `TimeRange` types
// using local time.
scada::base::Time AlignTime(scada::base::Time time,
                            scada::base::TimeDelta interval) {
  return time - (time - scada::base::Time::UnixEpoch()) % interval;
}

}  // namespace

scada::DateTimeRange ToDateTimeRangeWithOpenRange(const TimeRange& time_range,
                                                  scada::base::Time now) {
  auto result = ToDateTimeRange(time_range, now);

  if (time_range.end.is_null() || time_range.end.is_max()) {
    result.second = scada::base::Time::Max();
  }

  return result;
}

scada::DateTimeRange ToDateTimeRange(const TimeRange& time_range,
                                     scada::base::Time now) {
  scada::base::Time from, to;

  switch (time_range.type) {
    case TimeRange::Type::Day:
      from = now.LocalMidnight();
      break;

    case TimeRange::Type::Week: {
      scada::base::Time cur = now.LocalMidnight();
      scada::base::Time::Exploded ts = {};
      cur.LocalExplode(&ts);
      // We need to start day of week from Monday instead of Sunday.
      unsigned day_of_week = (ts.day_of_week + 6) % 7;
      from = cur - scada::base::TimeDelta::FromDays(day_of_week);
      break;
    }

    case TimeRange::Type::Month: {
      scada::base::Time::Exploded ts;
      now.LocalMidnight().LocalExplode(&ts);
      ts.day_of_month = 1;
      scada::base::Time::FromLocalExploded(ts, &from);
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
  if (to.is_null())
    to = now;

  if (time_range.dates) {
    from = from.LocalMidnight();
    to = to.LocalMidnight() + scada::base::TimeDelta::FromDays(1);
  }

  if (from.is_null() || from >= to) {
    to = now;
    from = to - scada::base::TimeDelta::FromHours(1);
  }

  scada::base::Check(!from.is_null());
  scada::base::Check(!to.is_null());
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
