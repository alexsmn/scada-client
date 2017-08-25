#pragma once

#include "base/time/time.h"

struct TimeRangeBound {
  TimeRangeBound()
      : date_only(true) {
  }
  TimeRangeBound(base::Time time, bool date_only)
      : time(time),
        date_only(date_only) {
  }
  
  base::Time GetStartTime() const {
    if (time.is_null())
      return base::Time();
    else if (date_only)
      return time.LocalMidnight() - base::TimeDelta::FromDays(1);
    else
      return time;
  }

  base::Time GetEndTime() const {
    if (time.is_null())
      return base::Time();
    else if (date_only)
      return time.LocalMidnight() + base::TimeDelta::FromDays(1);
    else
      return time;
  }

  base::Time time;
  bool date_only;
};

struct TimeRange {
  TimeRange() { }
  TimeRange(const TimeRangeBound& start, const TimeRangeBound& end)
      : start(start),
        end(end) {
  }

  TimeRangeBound start;
  TimeRangeBound end;
};
