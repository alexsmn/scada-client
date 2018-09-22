#include "components/print_preview/print_preview.h"

#include "services/dialog_service.h"
#include "services/print_service.h"

#include <QPrintPreviewDialog>

void ShowPrintPreviewDialog(DialogService& dialog_service,
                            PrintService& print_service,
                            const PrintHandler& print_handler) {
  QPrintPreviewDialog dialog{&print_service.printer,
                             dialog_service.GetParentWidget()};
  QObject::connect(&dialog, &QPrintPreviewDialog::paintRequested,
                   print_handler);
  dialog.exec();
}
