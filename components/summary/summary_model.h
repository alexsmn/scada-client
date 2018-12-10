#pragma once

#include <memory>

#include "base/time/time.h"
#include "common/node_ref.h"
#include "contents_model.h"
#include "core/configuration_types.h"
#include "time_model.h"
#include "ui/base/models/grid_model.h"

namespace rt {
class TimedDataSpec;
}

namespace scada {
class DataValue;
}

class NodeService;
class TimedDataService;
class WindowDefinition;

struct SummaryModelContext {
  NodeService& node_service_;
  TimedDataService& timed_data_service_;
};

class SummaryModel : private SummaryModelContext,
                     public ui::GridModel,
                     public ContentsModel,
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
  void DeleteColumn(int index);
  int FindColumn(const scada::NodeId& node_id, int starting_index = 0) const;

  void Load(const WindowDefinition& definition);
  void Save(WindowDefinition& definition);

  ui::HeaderModel& row_model();
  ui::HeaderModel& column_model();

  TimedDataService& timed_data_service() { return timed_data_service_; }

  const scada::DataValue& data_value(int row, int column) const;
  const rt::TimedDataSpec& timed_data(int column) const;

  // ui::GridModel
  virtual void GetCell(ui::GridCell& cell) override;

  // ContentsModel
  virtual void AddContainedItem(const scada::NodeId& node_id,
                                unsigned flags) override;
  virtual void RemoveContainedItem(const scada::NodeId& node_id) override;
  virtual NodeIdSet GetContainedItems() const override;

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
