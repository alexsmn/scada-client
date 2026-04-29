#include "components/write/write_dialog.h"

#include "aui/wt/dialog_stub.h"
#include "components/write/write_model.h"

promise<void> ExecuteWriteDialog(DialogService& dialog_service,
                                 WriteContext&& context) {
  return aui::wt::MakeUnsupportedDialogPromise<void>();
}
