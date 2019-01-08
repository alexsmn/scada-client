#pragma once

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

  struct TableExportData {
    ui::TableModel& model;
    const std::vector<ui::TableColumn>& columns;
  };

  struct GridExportData {
    ui::GridModel& model;
    ui::HeaderModel& rows;
    ui::HeaderModel& columns;
  };

  using ExportData = std::variant<TableExportData, GridExportData>;

  virtual ExportData GetExportData() = 0;
};
