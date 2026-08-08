#include "modules/time_range/time_range_dialog.h"

#include "aui/dialog_service.h"
#include "aui/wt/dialog_stub.h"

Awaitable<scada::RelativeTimeRange> ShowTimeRangeDialog(DialogService& dialog_service,
                                         TimeRangeContext&& context) {
  return scada::aui::wt::MakeUnsupportedDialogAwaitable<scada::RelativeTimeRange>();
}
