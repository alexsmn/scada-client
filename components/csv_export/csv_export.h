#pragma once

#include <string>

class DialogService;
struct CsvExportParams;

bool ShowCsvExportDialog(DialogService& dialog_service,
                         CsvExportParams& params);
