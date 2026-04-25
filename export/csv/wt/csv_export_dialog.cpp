#include "export/csv/csv_export.h"

#include "aui/wt/dialog_stub.h"

promise<CsvExportParams> ShowCsvExportDialog(DialogService& dialog_service,
                                             Profile& profile) {
  return aui::wt::MakeUnsupportedDialogPromise<CsvExportParams>();
}
