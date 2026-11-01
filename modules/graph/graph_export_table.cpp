#include "graph/graph_export_table.h"

#include "aui/translation.h"
#include "base/format_time.h"
#include "base/utf_convert.h"

#include <algorithm>

GraphExportTable::GraphExportTable(std::span<const GraphExportSeries> series,
                                   scada::Time from,
                                   scada::Time to,
                                   bool utc)
    : utc_{utc} {
  columns_.push_back(scada::aui::TableColumn{
      0, Translate("Time"), 150, scada::aui::TableColumn::LEFT,
      scada::aui::TableColumn::DataType::DateTime});
  for (size_t i = 0; i < series.size(); ++i) {
    columns_.push_back(
        scada::aui::TableColumn{static_cast<int>(i + 1), series[i].title, 100,
                                scada::aui::TableColumn::RIGHT,
                                scada::aui::TableColumn::DataType::General});
  }

  // Every recorded moment in range, from every series.
  std::vector<scada::Time> times;
  for (const auto& s : series) {
    for (const auto& value : s.values) {
      if (value.source_timestamp >= from && value.source_timestamp <= to)
        times.push_back(value.source_timestamp);
    }
  }
  std::ranges::sort(times);
  times.erase(std::unique(times.begin(), times.end()), times.end());

  rows_.reserve(times.size());
  for (scada::Time time : times)
    rows_.push_back(
        Row{time, std::vector<std::optional<std::u16string>>(series.size())});

  for (size_t i = 0; i < series.size(); ++i) {
    for (const auto& value : series[i].values) {
      auto row = std::ranges::lower_bound(rows_, value.source_timestamp,
                                          std::less{}, &Row::time);
      if (row != rows_.end() && row->time == value.source_timestamp)
        row->cells[i] = series[i].format(value);
    }
  }
}

int GraphExportTable::GetRowCount() {
  return static_cast<int>(rows_.size());
}

void GraphExportTable::GetCell(scada::aui::TableCell& cell) {
  const Row& row = rows_[cell.row];
  if (cell.column_id == 0) {
    cell.text = UtfConvert<char16_t>(FormatTime(
        row.time, TIME_FORMAT_DATE | TIME_FORMAT_TIME | TIME_FORMAT_MSEC |
                      (utc_ ? TIME_FORMAT_UTC : 0)));
    return;
  }
  const size_t index = static_cast<size_t>(cell.column_id - 1);
  if (index < row.cells.size() && row.cells[index])
    cell.text = *row.cells[index];
}
