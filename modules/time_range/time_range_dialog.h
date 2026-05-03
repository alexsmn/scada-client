#pragma once

#include "base/awaitable.h"
#include "base/time_range.h"

class DialogService;
class Profile;

struct TimeRangeContext {
  Profile& profile_;
  TimeRange time_range_;
  bool time_required_;
};

Awaitable<TimeRange> ShowTimeRangeDialog(DialogService& dialog_service,
                                         TimeRangeContext&& context);
