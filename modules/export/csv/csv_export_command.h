#pragma once

#include "base/any_executor.h"

#include "base/awaitable.h"
#include "export/csv/csv_export_util.h"

#include <functional>
#include <string>

class DialogService;
class ExportModel;
class Profile;

using CsvExportDialogRunner =
    std::function<Awaitable<CsvExportParams>(DialogService&, Profile&)>;

struct CsvExportContext {
  AnyExecutor executor_;
  DialogService& dialog_service_;
  Profile& profile_;
  ExportModel& export_model_;
  std::u16string window_title_;
  CsvExportDialogRunner show_csv_export_dialog;
};

Awaitable<void> RunCsvExport(CsvExportContext&& context);
