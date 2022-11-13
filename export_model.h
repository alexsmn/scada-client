#pragma once

#include "controls/models/table_column.h"
#include "controls/models/table_model.h"

#include <optional>
#include <variant>
#include <vector>

namespace aui {
class GridModel;
class HeaderModel;
class TableModel;
struct TableColumn;
}  // namespace aui

class ExportModel {
 public:
  virtual ~ExportModel() {}

  struct Range {
    int first;
    int count;
  };

  struct TableExportData {
    aui::TableModel& model;
    const std::vector<aui::TableColumn>& columns;
    std::optional<Range> row_range;

    Range GetRowRange() const {
      return row_range.has_value() ? *row_range : Range{0, model.GetRowCount()};
    }
  };

  struct GridExportData {
    aui::TableColumn row_title_column;
    aui::GridModel& model;
    aui::HeaderModel& rows;
    aui::HeaderModel& columns;
  };

  using ExportData = std::variant<TableExportData, GridExportData>;

  virtual ExportData GetExportData() = 0;
};
