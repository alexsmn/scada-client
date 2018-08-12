#pragma once

#include "time_range.h"

class DialogService;
class Profile;

struct TimeRangeContext {
  Profile& profile_;
  TimeRange& time_range_;
  bool time_required_;
};

bool ShowTimeRangeDialog(DialogService& dialog_service,
                         TimeRangeContext&& context);
