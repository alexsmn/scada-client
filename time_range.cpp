#include "time_range.h"

#include "base/strings/string_piece.h"

namespace {

const base::StringPiece kTimeRangeStrings[] = {
    "Custom",
    "Day",
    "Week",
    "Month",
};

static_assert(std::size(kTimeRangeStrings) ==
              static_cast<size_t>(TimeRange::Type::Count));

inline bool CompareBounds(base::Time a, base::Time b, bool dates) {
  if (dates)
    return a.LocalMidnight() == b.LocalMidnight();
  else
    return a == b;
}

}  // namespace

bool operator==(const TimeRange& a, const TimeRange& b) {
  if (a.type != b.type)
    return false;

  if (a.dates != b.dates)
    return false;

  return CompareBounds(a.start, b.start, a.dates) &&
         CompareBounds(a.end, b.end, b.dates);
}

std::pair<base::Time, base::Time> GetTimeRangeBounds(
    const TimeRange& time_range) {
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
             ? kTimeRangeStrings[index].as_string()
             : "Unknown";
}

std::string ToString(const TimeRange& time_range) {
  return ToString(time_range.type);
}

TimeRange::Type ParseTimeRangeType(base::StringPiece str) {
  auto i = std::find(std::begin(kTimeRangeStrings), std::end(kTimeRangeStrings),
                     str);
  return i != std::end(kTimeRangeStrings)
             ? static_cast<TimeRange::Type>(i - std::begin(kTimeRangeStrings))
             : TimeRange::Type::Count;
}
