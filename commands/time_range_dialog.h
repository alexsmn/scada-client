#pragma once

#include "time_range.h"

class Profile;

bool ShowTimeRangeDialog(Profile& profile,
                         TimeRange& range,
                         bool time_required);
