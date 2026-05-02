#include "print/service/print_service.h"

#include "aui/dialog_service.h"
#include "aui/qt/dialog_util.h"

#include <QPrintPreviewDialog>

void PrintService::ShowPrintPreviewDialog(
    DialogService& dialog_service,
    const PrintHandler& print_handler) {
  auto dialog = std::make_unique<QPrintPreviewDialog>(
      &printer, dialog_service.GetParentWidget());
  QObject::connect(dialog.get(), &QPrintPreviewDialog::paintRequested,
                   print_handler);
  StartModalDialog(std::move(dialog));
}
