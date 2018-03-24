#pragma once

#include "base/time/time.h"
#include "common_resources.h"

#include <cassert>

struct TimeRange {
  TimeRange() {}
  explicit TimeRange(unsigned command_id) : command_id{command_id} {}
  TimeRange(base::Time start, base::Time end)
      : start{start}, end{end}, command_id{ID_TIME_RANGE_CUSTOM} {}

  unsigned command_id = ID_TIME_RANGE_DAY;
  base::Time start;
  base::Time end;
  bool dates = false;
};

bool operator==(const TimeRange& a, const TimeRange& b);

std::pair<base::Time, base::Time> GetTimeRangeBounds(
    const TimeRange& time_range);

const char* FormatTimeRange(unsigned mode);
