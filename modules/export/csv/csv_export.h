#pragma once

#include "base/awaitable.h"
#include "export/csv/csv_export_util.h"

#include <string>

class DialogService;
class Profile;

// Asks for the CSV options. `can_expand` tells the dialog whether the data
// being exported has grouped rows to expand; the option is hidden when it does
// not, rather than offered with no effect.
Awaitable<CsvExportParams> ShowCsvExportDialog(DialogService& dialog_service,
                                               Profile& profile,
                                               bool can_expand);
