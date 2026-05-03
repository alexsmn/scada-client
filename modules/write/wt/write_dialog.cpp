#include "modules/write/write_dialog.h"

#include "aui/wt/dialog_stub.h"
#include "modules/write/write_model.h"

Awaitable<void> ExecuteWriteDialog(DialogService& dialog_service,
                                   WriteContext&& context) {
  return aui::wt::MakeUnsupportedDialogAwaitable<void>();
}
