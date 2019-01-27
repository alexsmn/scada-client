#pragma once

#include "ui/base/models/table_column.h"
#include "ui/base/models/table_model.h"

#include <optional>
#include <variant>
#include <vector>

namespace ui {
class GridModel;
class HeaderModel;
class TableModel;
struct TableColumn;
}  // namespace ui

class ExportModel {
 public:
  virtual ~ExportModel() {}

  struct Range {
    int first;
    int count;
  };

  struct TableExportData {
    ui::TableModel& model;
    const std::vector<ui::TableColumn>& columns;
    std::optional<Range> row_range;

    Range GetRowRange() const {
      return row_range.has_value() ? *row_range : Range{0, model.GetRowCount()};
    }
  };

  struct GridExportData {
    ui::TableColumn row_title_column;
    ui::GridModel& model;
    ui::HeaderModel& rows;
    ui::HeaderModel& columns;
  };

  using ExportData = std::variant<TableExportData, GridExportData>;

  virtual ExportData GetExportData() = 0;
};
