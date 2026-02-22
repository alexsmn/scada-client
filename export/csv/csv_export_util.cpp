#include "export/csv/csv_export_util.h"

#include "aui/models/grid_model.h"
#include "aui/models/header_model.h"
#include "aui/models/table_column.h"
#include "aui/models/table_model.h"
#include "base/csv_writer.h"
#include "base/excel.h"
#include "base/json.h"
#include "base/utf_convert.h"
#include "base/value_util.h"
#include "scada/variant.h"

#include <fstream>

namespace {

bool Convert(const scada::Variant& source, base::win::ScopedVariant& target) {
  assert(!source.is_array());
  if (source.is_array())
    return false;

  static_assert(scada::Variant::COUNT == 19);

  switch (source.type()) {
    case scada::Variant::Type::EMPTY:
      target.Reset();
      return true;
    case scada::Variant::Type::BOOL:
      target.Set(source.get<scada::Boolean>());
      return true;
    case scada::Variant::Type::INT8:
      target.Set(source.get<scada::Int8>());
      return true;
    case scada::Variant::Type::UINT8:
      target.Set(source.get<scada::UInt8>());
      return true;
    case scada::Variant::Type::INT16:
      target.Set(source.get<scada::Int16>());
      return true;
    case scada::Variant::Type::UINT16:
      target.Set(source.get<scada::UInt16>());
      return true;
    case scada::Variant::Type::INT32:
      target.Set(source.get<scada::Int32>());
      return true;
    case scada::Variant::Type::UINT32:
      target.Set(source.get<scada::UInt32>());
      return true;
    case scada::Variant::Type::INT64:
      target.Set(source.get<scada::Int64>());
      return true;
    case scada::Variant::Type::UINT64:
      target.Set(source.get<scada::UInt64>());
      return true;
    case scada::Variant::Type::DOUBLE:
      target.Set(source.get<scada::Double>());
      return true;
    case scada::Variant::STRING:
      target.Set(UtfConvert<wchar_t>(source.get<scada::String>()).c_str());
      return true;
    case scada::Variant::LOCALIZED_TEXT:
      target.Set(source.get<scada::LocalizedText>().c_str());
      return true;
    case scada::Variant::DATE_TIME:
      target.Set(static_cast<DATE>(source.get<scada::DateTime>().ToDoubleT()));
      return true;
    default:
      assert(false);
      return false;
  }
}

}  // namespace

void ExportToCsv(ExportModel::TableExportData& table,
                 const CsvExportParams& params,
                 const std::filesystem::path& path) {
  std::ofstream stream{path};
  CsvWriter writer{stream};
  writer.unicode = params.unicode;
  writer.delimiter = params.delimiter;
  writer.quote = params.quote;

  writer.StartRow();
  for (int i = 0; i < static_cast<int>(table.columns.size()); ++i)
    writer.WriteCell(table.columns[i].title);

  const auto row_range = table.GetRowRange();
  for (int i = 0; i < row_range.count; ++i) {
    writer.StartRow();

    for (int j = 0; j < static_cast<int>(table.columns.size()); ++j) {
      auto column_id = table.columns[j].id;
      auto text = table.model.GetCellText(row_range.first + i, column_id);
      writer.WriteCell(text);
    }
  }
}

void ExportToCsv(ExportModel::GridExportData& grid,
                 const CsvExportParams& params,
                 const std::filesystem::path& path) {
  std::ofstream stream{path};
  CsvWriter writer{stream};
  writer.unicode = params.unicode;
  writer.delimiter = params.delimiter;
  writer.quote = params.quote;

  writer.StartRow();
  writer.WriteCell(grid.row_title_column.title);
  for (int i = 0; i < grid.columns.GetCount(); ++i)
    writer.WriteCell(grid.columns.GetTitle(i));

  for (int i = 0; i < grid.rows.GetCount(); ++i) {
    writer.StartRow();
    writer.WriteCell(grid.rows.GetTitle(i));
    for (int j = 0; j < grid.columns.GetCount(); ++j) {
      auto text = grid.model.GetCellText(i, j);
      writer.WriteCell(text);
    }
  }
}

void ExportToExcel(ExportModel::TableExportData& table,
                   ExcelSheetModel& sheet) {
  const auto row_range = table.GetRowRange();
  const int column_count = static_cast<int>(table.columns.size());

  sheet.SetDataSize(row_range.count + 1, column_count);

  // Column titles.
  for (int i = 0; i < column_count; ++i) {
    const auto& title = table.columns[i].title;
    sheet.SetData(1, 1 + i, UtfConvert<wchar_t>(title));
  }

  // Cells.
  base::win::ScopedVariant data;
  for (int i = 0; i < row_range.count; ++i) {
    for (int j = 0; j < column_count; ++j) {
      auto column_id = table.columns[j].id;
      auto text = table.model.GetCellText(row_range.first + i, column_id);
      sheet.SetData(2 + i, 1 + j, UtfConvert<wchar_t>(text));
    }
  }
}

void ExportToExcel(ExportModel::GridExportData& grid, ExcelSheetModel& sheet) {
  int row_count = grid.rows.GetCount();
  int column_count = grid.columns.GetCount();

  sheet.SetDataSize(row_count + 1, column_count + 1);

  // Column titles.
  sheet.SetData(1, 1, UtfConvert<wchar_t>(grid.row_title_column.title));
  for (int i = 0; i < column_count; ++i) {
    auto title = grid.columns.GetTitle(i);
    sheet.SetData(1, 2 + i, UtfConvert<wchar_t>(title));
  }

  // Row titles.
  for (int i = 0; i < row_count; ++i) {
    auto title = grid.rows.GetTitle(i);
    sheet.SetData(2 + i, 1, UtfConvert<wchar_t>(title));
  }

  // Cells.
  base::win::ScopedVariant data;
  for (int i = 0; i < row_count; ++i) {
    for (int j = 0; j < column_count; ++j) {
      auto text = grid.model.GetCellText(i, j);
      sheet.SetData(2 + i, 2 + j, UtfConvert<wchar_t>(text));
    }
  }
}

base::Value ToJson(const CsvExportParams& params) {
  base::Value result{base::Value::Type::DICTIONARY};
  SetKey(result, "unicode", params.unicode);
  SetKey(result, "delimiter", std::string{params.delimiter});
  SetKey(result, "quote", std::string{params.quote});
  return result;
}

template <>
std::optional<CsvExportParams> FromJson(const base::Value& value) {
  CsvExportParams params;

  params.unicode = GetBool(value, "unicode", params.unicode);

  auto delimiter = GetString(value, "delimiter");
  if (delimiter.size() == 1)
    params.delimiter = delimiter.front();

  auto quote = GetString(value, "quote");
  if (quote.size() == 1)
    params.quote = quote.front();

  return params;
}
