#pragma once

#include "base/values.h"
#include "controller/export_model.h"

#include <filesystem>

class ExcelSheetModel;

struct CsvExportParams {
  bool unicode = false;
  char delimiter = ',';
  char quote = '"';
};

void ExportToCsv(ExportModel::TableExportData& table,
                 const CsvExportParams& params,
                 const std::filesystem::path& path);
void ExportToCsv(ExportModel::GridExportData& grid,
                 const CsvExportParams& params,
                 const std::filesystem::path& path);

void ExportToExcel(ExportModel::TableExportData& table, ExcelSheetModel& sheet);
void ExportToExcel(ExportModel::GridExportData& grid, ExcelSheetModel& sheet);

base::Value ToJson(const CsvExportParams& params);
