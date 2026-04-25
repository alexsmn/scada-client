#include "components/time_range/time_range_dialog.h"

#include "aui/dialog_service.h"
#include "aui/wt/dialog_stub.h"

promise<TimeRange> ShowTimeRangeDialog(DialogService& dialog_service,
                                       TimeRangeContext&& context) {
  return aui::wt::MakeUnsupportedDialogPromise<TimeRange>();
}
