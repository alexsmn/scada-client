#pragma once

#include "base/csv_writer.h"
#include "export/export_model.h"
#include <boost/json.hpp>

#include <filesystem>

class ExcelSheetModel;

struct CsvExportParams {
  // UTF-8 with a byte-order mark when set; otherwise the system encoding
  // (SystemCsvEncoding(): the ANSI code page on Windows).
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

// Writes `table` or `grid` to `path`. `system_encoding` is what
// `params.unicode == false` means; tests replace it to exercise a code page
// the host does not have.
void ExportToCsv(ExportModel::TableExportData& table,
                 const CsvExportParams& params,
                 const std::filesystem::path& path,
                 const CsvEncoding& system_encoding = SystemCsvEncoding());
void ExportToCsv(ExportModel::GridExportData& grid,
                 const CsvExportParams& params,
                 const std::filesystem::path& path,
                 const CsvEncoding& system_encoding = SystemCsvEncoding());

void ExportToExcel(ExportModel::TableExportData& table, ExcelSheetModel& sheet);
void ExportToExcel(ExportModel::GridExportData& grid, ExcelSheetModel& sheet);

boost::json::value ToJson(const CsvExportParams& params);
