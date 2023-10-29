#include "components/time_range/time_range_dialog.h"

#include "aui/dialog_service.h"

promise<TimeRange> ShowTimeRangeDialog(DialogService& dialog_service,
                                       TimeRangeContext&& context) {
  return make_rejected_promise<TimeRange>(std::exception{});
}
