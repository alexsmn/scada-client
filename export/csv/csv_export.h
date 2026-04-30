#pragma once

#include "base/awaitable.h"
#include "export/csv/csv_export_util.h"

#include <string>

class DialogService;
class Profile;

Awaitable<CsvExportParams> ShowCsvExportDialog(DialogService& dialog_service,
                                               Profile& profile);
