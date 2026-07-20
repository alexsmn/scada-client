#pragma once

#include "aui/models/table_column.h"
#include "aui/models/table_model.h"

#include <optional>
#include <variant>
#include <vector>

namespace scada::aui {
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
    scada::aui::TableModel& model;
    const std::vector<scada::aui::TableColumn>& columns;
    std::optional<Range> row_range;

    Range GetRowRange() const {
      return row_range.has_value() ? *row_range : Range{0, model.GetRowCount()};
    }
  };

  struct GridExportData {
    scada::aui::TableColumn row_title_column;
    scada::aui::GridModel& model;
    scada::aui::HeaderModel& rows;
    scada::aui::HeaderModel& columns;
  };

  using ExportData = std::variant<TableExportData, GridExportData>;

  virtual ExportData GetExportData() = 0;
};
