#pragma once

#include <memory>

#include "base/time/time.h"
#include "core/configuration_types.h"
#include "ui/base/models/grid_model.h"

namespace scada {
class DataValue;
}

class TimedDataService;
class WindowDefinition;

class SummaryModel : public ui::GridModel {
 public:
  explicit SummaryModel(TimedDataService& timed_data_service);

  TimedDataService& timed_data_service() { return timed_data_service_; }

  struct Times {
    base::Time start_time;
    base::Time end_time;
    base::TimeDelta interval;
  };

  const Times& times() const { return times_; }
  void SetTimes(const Times& times);

  int AddColumn(const std::string& formula);

  void Load(const WindowDefinition& definition);
  void Save(WindowDefinition& definition);

  ui::HeaderModel& row_model();
  ui::HeaderModel& column_model();

  // ui::GridModel
  virtual void GetCell(ui::GridCell& cell) override;

 private:
  class Cell;
  class Column;
  class ColumnModel;
  class RowModel;

  base::Time GetRowTime(int row) const;
  int GetRowForTime(base::Time time) const;

  void OnCellChanged(int column, int row);
  void OnColumnChanged(int column);
  void OnColumnTitleChanged(int column);

  TimedDataService& timed_data_service_;

  std::vector<std::unique_ptr<Column>> columns_;

  Times times_;

  size_t row_count_;

  std::unique_ptr<RowModel> row_model_;
  std::unique_ptr<ColumnModel> column_model_;
};
