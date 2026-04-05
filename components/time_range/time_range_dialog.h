#pragma once

#include "base/promise.h"
#include "base/time_range.h"

class DialogService;
class Profile;

struct TimeRangeContext {
  Profile& profile_;
  TimeRange time_range_;
  bool time_required_;
};

promise<TimeRange> ShowTimeRangeDialog(DialogService& dialog_service,
                                       TimeRangeContext&& context);
