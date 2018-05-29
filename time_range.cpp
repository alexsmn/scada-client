#include "time_range.h"

namespace {

inline bool CompareBounds(base::Time a, base::Time b, bool dates) {
  if (dates)
    return a.LocalMidnight() == b.LocalMidnight();
  else
    return a == b;
}

}  // namespace

bool operator==(const TimeRange& a, const TimeRange& b) {
  if (a.command_id != b.command_id)
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

  switch (time_range.command_id) {
    case ID_TIME_RANGE_DAY:
      from = now.LocalMidnight();
      break;

    case ID_TIME_RANGE_WEEK: {
      base::Time cur = now.LocalMidnight();
      base::Time::Exploded ts = {};
      cur.LocalExplode(&ts);
      // We need to start day of week from Monday instead of Sunday.
      unsigned day_of_week = (ts.day_of_week + 6) % 7;
      from = cur - base::TimeDelta::FromDays(day_of_week);
      break;
    }

    case ID_TIME_RANGE_MONTH: {
      base::Time::Exploded ts;
      now.LocalMidnight().LocalExplode(&ts);
      ts.day_of_month = 1;
      base::Time::FromLocalExploded(ts, &from);
      break;
    }

    case ID_TIME_RANGE_CUSTOM:
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

const char* FormatTimeRange(unsigned mode) {
  switch (mode) {
    case ID_TIME_RANGE_DAY:
      return "Day";
    case ID_TIME_RANGE_WEEK:
      return "Week";
    case ID_TIME_RANGE_MONTH:
      return "Month";
    case ID_TIME_RANGE_CUSTOM:
      return "Custom";
    default:
      return "Current";
  }
}
