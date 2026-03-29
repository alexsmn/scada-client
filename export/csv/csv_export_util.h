#pragma once

#include <boost/json.hpp>
#include "export/export_model.h"

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

boost::json::value ToJson(const CsvExportParams& params);
