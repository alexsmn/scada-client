#include "components/csv_export/csv_export.h"

promise<CsvExportParams> ShowCsvExportDialog(DialogService& dialog_service,
                                             Profile& profile) {
  return make_rejected_promise<CsvExportParams>(std::exception{});
}
