#pragma once

#include "base/promise.h"
#include "export_util.h"

#include <string>

class DialogService;
class Profile;

promise<CsvExportParams> ShowCsvExportDialog(DialogService& dialog_service,
                                             Profile& profile);
