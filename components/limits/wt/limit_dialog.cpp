#include "components/limits/limit_dialog.h"

#include "components/limits/limit_model.h"
#include "aui/dialog_service.h"

promise<void> ShowLimitsDialog(DialogService& dialog_service,
                               LimitDialogContext&& context) {
  return make_resolved_promise();
}
