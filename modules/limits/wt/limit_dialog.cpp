#include "modules/limits/limit_dialog.h"

#include "aui/dialog_service.h"
#include "aui/wt/dialog_stub.h"
#include "modules/limits/limit_model.h"

Awaitable<void> ShowLimitsDialog(DialogService& dialog_service,
                                 LimitDialogContext&& context) {
  return aui::wt::MakeUnsupportedDialogAwaitable<void>();
}
