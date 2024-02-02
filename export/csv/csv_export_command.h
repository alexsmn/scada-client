#pragma once

#include "base/promise.h"

#include <string>

class DialogService;
class Executor;
class ExportModel;
class Profile;

struct CsvExportContext {
  std::shared_ptr<Executor> executor_;
  DialogService& dialog_service_;
  Profile& profile_;
  ExportModel& export_model_;
  std::u16string window_title_;
};

promise<void> RunCsvExport(CsvExportContext&& context);
