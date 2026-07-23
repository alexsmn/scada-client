#pragma once

#include "base/awaitable.h"
#include "base/relative_time_range.h"

class DialogService;
class Profile;

struct TimeRangeContext {
  Profile& profile_;
  scada::RelativeTimeRange time_range_;
  bool time_required_;
};

Awaitable<scada::RelativeTimeRange> ShowTimeRangeDialog(DialogService& dialog_service,
                                         TimeRangeContext&& context);
