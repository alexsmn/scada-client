#pragma once

#include "base/relative_time_range.h"

class TimeModel {
 public:
  virtual ~TimeModel() {}

  virtual scada::RelativeTimeRange GetTimeRange() const = 0;
  virtual void SetTimeRange(const scada::RelativeTimeRange& time_range) = 0;

  // TODO: Describe what this is for.
  virtual bool IsTimeRequired() const { return false; }
};
