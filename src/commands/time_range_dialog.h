#pragma once

#include "client/time_range.h"

class DialogService;

bool ShowTimeRangeDialog(DialogService& dialog_service, TimeRange& range);
