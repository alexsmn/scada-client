#include "export_util.h"

#include "base/excel.h"
#include "base/strings/sys_string_conversions.h"
#include "base/table_writer.h"
#include "core/variant.h"
#include "ui/base/models/grid_model.h"
#include "ui/base/models/header_model.h"
#include "ui/base/models/table_column.h"
#include "ui/base/models/table_model.h"

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
      target.Set(base::SysNativeMBToWide(source.get<scada::String>()).c_str());
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
                 const std::filesystem::path& path) {
  std::ofstream stream{path};
  TableWriter writer{stream};

  writer.StartRow();
  for (int i = 0; i < static_cast<int>(table.columns.size()); ++i)
    writer.WriteCell(table.columns[i].title);

  for (int i = 0; i < table.model.GetRowCount(); ++i) {
    writer.StartRow();

    for (int j = 0; j < static_cast<int>(table.columns.size()); ++j) {
      auto column_id = table.columns[j].id;
      auto text = table.model.GetCellText(i, column_id);
      writer.WriteCell(text);
    }
  }
}

void ExportToCsv(ExportModel::GridExportData& grid,
                 const std::filesystem::path& path) {
  std::ofstream stream{path};
  TableWriter writer{stream};

  writer.StartRow();
  writer.WriteCell({});
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
  int row_count = table.model.GetRowCount();
  int column_count = static_cast<int>(table.columns.size());

  sheet.SetDataSize(row_count + 1, column_count);

  // Column titles.
  for (int i = 0; i < column_count; ++i) {
    const auto& title = table.columns[i].title;
    sheet.SetData(1, 1 + i, title);
  }

  // Cells.
  base::win::ScopedVariant data;
  for (int i = 0; i < row_count; ++i) {
    for (int j = 0; j < column_count; ++j) {
      auto column_id = table.columns[j].id;
      auto text = table.model.GetCellText(i, column_id);
      sheet.SetData(2 + i, 1 + j, std::move(text));
    }
  }
}

void ExportToExcel(ExportModel::GridExportData& grid, ExcelSheetModel& sheet) {
  int row_count = grid.rows.GetCount();
  int column_count = grid.columns.GetCount();

  sheet.SetDataSize(row_count + 1, column_count + 1);

  // Column titles.
  for (int i = 0; i < column_count; ++i) {
    const auto& title = grid.columns.GetTitle(i);
    sheet.SetData(1, 2 + i, base::win::ScopedVariant(title.c_str()));
  }

  // Row titles.
  for (int i = 0; i < row_count; ++i) {
    const auto& title = grid.rows.GetTitle(i);
    sheet.SetData(2 + i, 1, base::win::ScopedVariant(title.c_str()));
  }

  // Cells.
  base::win::ScopedVariant data;
  for (int i = 0; i < row_count; ++i) {
    for (int j = 0; j < column_count; ++j) {
      auto text = grid.model.GetCellText(i, j);
      sheet.SetData(2 + i, 2 + j, std::move(text));
    }
  }
}
