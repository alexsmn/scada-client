#include "components/limits/limit_dialog.h"

#include "aui/dialog_service.h"
#include "aui/wt/dialog_stub.h"
#include "components/limits/limit_model.h"

promise<void> ShowLimitsDialog(DialogService& dialog_service,
                               LimitDialogContext&& context) {
  return aui::wt::MakeUnsupportedDialogPromise<void>();
}
