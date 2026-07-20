#include "export/csv/csv_export.h"

#include "aui/wt/dialog_stub.h"

Awaitable<CsvExportParams> ShowCsvExportDialog(DialogService& dialog_service,
                                               Profile& profile) {
  return scada::aui::wt::MakeUnsupportedDialogAwaitable<CsvExportParams>();
}
