#pragma once

#include "base/time_range.h"

class TimeModel {
 public:
  virtual ~TimeModel() {}

  virtual TimeRange GetTimeRange() const = 0;
  virtual void SetTimeRange(const TimeRange& time_range) = 0;

  // TODO: Describe what this is for.
  virtual bool IsTimeRequired() const { return false; }
};
