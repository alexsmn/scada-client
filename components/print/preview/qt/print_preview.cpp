#include "print/preview/print_preview.h"

#include "aui/dialog_service.h"
#include "aui/qt/dialog_util.h"
#include "print/service/print_service.h"

#include <QPrintPreviewDialog>

void ShowPrintPreviewDialog(DialogService& dialog_service,
                            PrintService& print_service,
                            const PrintHandler& print_handler) {
  auto dialog = std::make_unique<QPrintPreviewDialog>(
      &print_service.printer, dialog_service.GetParentWidget());
  QObject::connect(dialog.get(), &QPrintPreviewDialog::paintRequested,
                   print_handler);
  StartModalDialog(std::move(dialog));
}
