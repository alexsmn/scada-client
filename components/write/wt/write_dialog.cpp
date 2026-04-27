#include "components/write/write_dialog.h"

#include "components/write/write_model.h"

promise<void> ExecuteWriteDialog(DialogService& dialog_service,
                                 WriteContext&& context) {
  return make_resolved_promise();
}
