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
}  // namespace scada::aui

class ExportModel {
 public:
  virtual ~ExportModel() {}

  struct Range {
    int first;
    int count;
  };

  struct TableExportData {
    // The rows as displayed.
    scada::aui::TableModel& model;
    const std::vector<scada::aui::TableColumn>& columns;
    std::optional<Range> row_range;

    // The same data with display-level grouping expanded — one entry per
    // underlying record rather than per drawn row (the event journal collapses
    // repeated alarms during a flood). Null when the view has nothing to
    // expand, which is most of them.
    scada::aui::TableModel* expanded_model = nullptr;

    bool can_expand() const { return expanded_model != nullptr; }

    // The model to read, given whether the caller wants the expanded form.
    // Falls back to the displayed rows when there is nothing to expand.
    scada::aui::TableModel& ModelFor(bool expanded) const {
      return expanded && expanded_model ? *expanded_model : model;
    }

    // `row_range` addresses the displayed rows, so it does not carry over to
    // the expanded form — expanding always covers everything.
    Range GetRowRange(bool expanded = false) const {
      if (expanded && expanded_model)
        return Range{0, expanded_model->GetRowCount()};
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
