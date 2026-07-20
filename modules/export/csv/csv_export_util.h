#pragma once

#include "export/export_model.h"
#include <boost/json.hpp>

#include <filesystem>

class ExcelSheetModel;

struct CsvExportParams {
  bool unicode = false;
  char delimiter = ',';
  char quote = '"';
  // Write one row per underlying record rather than per displayed row, where
  // the view groups them (the event journal collapses repeated alarms during a
  // flood). Defaults on: a spreadsheet is a record of what happened, and counts
  // can be recovered from expanded rows but the rows cannot be recovered from a
  // count. Ignored by views that have nothing to expand.
  bool expand_groups = true;
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
