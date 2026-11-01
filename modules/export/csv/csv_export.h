#pragma once

#include "base/awaitable.h"
#include "export/csv/csv_export_util.h"

#include <string>

class DialogService;
class Profile;

// Asks for the CSV options. `can_expand` tells the dialog whether the data
// being exported has grouped rows to expand; the option is hidden when it does
// not, rather than offered with no effect.
// The delimiter a first export offers on this machine: the Windows list
// separator, or the one ChooseCsvDelimiter() derives from the decimal
// separator of the system locale elsewhere.
char SystemCsvDelimiter();

Awaitable<CsvExportParams> ShowCsvExportDialog(DialogService& dialog_service,
                                               Profile& profile,
                                               bool can_expand);
