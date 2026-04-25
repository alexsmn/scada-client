#pragma once

#include "base/promise.h"
#include "export/csv/csv_export_util.h"

#include <functional>
#include <string>

class DialogService;
class Executor;
class ExportModel;
class Profile;

using CsvExportDialogRunner =
    std::function<promise<CsvExportParams>(DialogService&, Profile&)>;

struct CsvExportContext {
  std::shared_ptr<Executor> executor_;
  DialogService& dialog_service_;
  Profile& profile_;
  ExportModel& export_model_;
  std::u16string window_title_;
  CsvExportDialogRunner show_csv_export_dialog;
};

promise<void> RunCsvExport(CsvExportContext&& context);
