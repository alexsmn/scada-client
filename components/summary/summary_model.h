#pragma once

#include <memory>

#include "base/time/time.h"
#include "core/configuration_types.h"
#include "time_model.h"
#include "ui/base/models/grid_model.h"

namespace scada {
class DataValue;
}

class TimedDataService;
class WindowDefinition;

struct SummaryModelContext {
  TimedDataService& timed_data_service_;
};

class SummaryModel : private SummaryModelContext,
                     public ui::GridModel,
                     public TimeModel {
 public:
  enum class AggregationFunction { Last, Avg, Min, Sum, Count, Max };

  explicit SummaryModel(SummaryModelContext&& context);

  const TimeRange& time_range() const { return time_range_; }

  base::TimeDelta interval() const { return interval_; }
  void SetInterval(base::TimeDelta interval);

  AggregationFunction aggregation_function() const {
    return aggregation_function_;
  }
  void SetAggregationFunction(AggregationFunction aggregation_function);

  void SetParams(const TimeRange& time_range,
                 base::TimeDelta interval,
                 AggregationFunction aggregation_function);

  int AddColumn(base::StringPiece formula);

  void Load(const WindowDefinition& definition);
  void Save(WindowDefinition& definition);

  ui::HeaderModel& row_model();
  ui::HeaderModel& column_model();

  TimedDataService& timed_data_service() { return timed_data_service_; }

  // ui::GridModel
  virtual void GetCell(ui::GridCell& cell) override;

  // TimeModel
  virtual TimeRange GetTimeRange() const override;
  virtual void SetTimeRange(const TimeRange& time_range) override;

  static bool IsCustomUnits(AggregationFunction aggregation_function);

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

  std::vector<std::unique_ptr<Column>> columns_;

  base::Time start_time_;
  base::Time end_time_;
  base::TimeDelta interval_;
  TimeRange time_range_;
  AggregationFunction aggregation_function_ = AggregationFunction::Last;

  size_t row_count_ = 0;

  std::unique_ptr<RowModel> row_model_;
  std::unique_ptr<ColumnModel> column_model_;
};
