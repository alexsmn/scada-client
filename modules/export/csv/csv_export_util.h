#pragma once

#include "base/csv_writer.h"
#include "export/export_model.h"
#include <boost/json.hpp>

#include <filesystem>
#include <string_view>

class ExcelSheetModel;

struct CsvExportParams {
  // UTF-8 with a byte-order mark when set; otherwise the system encoding
  // (SystemCsvEncoding(): the ANSI code page on Windows).
  bool unicode = false;
  // The dialog's first-use default is SystemCsvDelimiter(), not this.
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

// The delimiter that lets the spreadsheet on this machine split the file into
// columns. Excel splits a CSV file on the system list separator, so a `,`
// file opened under Russian regional settings arrives as one column (backlog
// 947). Takes |list_separator| when it is a usable single character; without
// one (it is a Windows setting) takes `;` when the decimal separator is `,`,
// as it is wherever the list separator is `;`, and `,` otherwise.
char ChooseCsvDelimiter(std::u16string_view list_separator,
                        std::u16string_view decimal_separator);

// The export options last saved in the profile (|stored|, possibly null),
// with |default_delimiter| where no delimiter was saved yet.
CsvExportParams LoadCsvExportParams(const boost::json::value& stored,
                                    char default_delimiter);
