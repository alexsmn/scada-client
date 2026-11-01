#pragma once

#include "aui/models/table_column.h"
#include "aui/models/table_model.h"
#include "base/lifetime.h"
#include "scada/data_value.h"
#include "scada/date_time.h"

#include <functional>
#include <optional>
#include <span>
#include <string>
#include <vector>

// One series of a graph, as the graph export reads it.
struct GraphExportSeries {
  std::u16string title;
  // The series' recorded values, ascending by source timestamp.
  std::span<const scada::DataValue> values;
  // Formats a value the way the series displays it, without quality.
  std::function<std::u16string(const scada::DataValue&)> format;
};

// The values of a graph's series over a time range, as a table for CSV and
// Excel export (backlog: the graph could be printed and saved only as a
// picture). One row per moment at which any series recorded a value, in
// ascending time; one column per series after the time column. A cell is
// filled only when that series recorded a value at that moment — the export
// carries what was recorded and does not invent values between records.
//
// The table is a snapshot taken at construction, so the export reads a
// consistent set even while history keeps arriving behind it.
class GraphExportTable : public scada::aui::TableModel {
 public:
  // Takes the values whose source timestamp lies in [from, to]. Times are
  // written in UTC when |utc| is set, in local time otherwise.
  GraphExportTable(std::span<const GraphExportSeries> series,
                   scada::Time from,
                   scada::Time to,
                   bool utc = false);

  // The time column followed by one column per series; ids are 0 for the
  // time and 1 + the series' index.
  const std::vector<scada::aui::TableColumn>& columns() const
      SCADA_LIFETIME_BOUND {
    return columns_;
  }

  // TableModel
  int GetRowCount() override;
  void GetCell(scada::aui::TableCell& cell) override;

 private:
  struct Row {
    scada::Time time;
    std::vector<std::optional<std::u16string>> cells;
  };

  std::vector<scada::aui::TableColumn> columns_;
  std::vector<Row> rows_;
  bool utc_ = false;
};
