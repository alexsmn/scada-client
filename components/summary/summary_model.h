#pragma once

#include <memory>

#include "base/time/time.h"
#include "contents_model.h"
#include "core/aggregate_filter.h"
#include "core/configuration_types.h"
#include "export_model.h"
#include "node_service/node_ref.h"
#include "time_model.h"
#include "ui/base/models/grid_model.h"

namespace scada {
class DataValue;
}

class NodeService;
class TimedDataService;
class TimedDataSpec;
class WindowDefinition;

struct SummaryModelContext {
  NodeService& node_service_;
  TimedDataService& timed_data_service_;
};

class SummaryModel : private SummaryModelContext,
                     public ui::GridModel,
                     public ContentsModel,
                     public TimeModel,
                     public ExportModel {
 public:
  explicit SummaryModel(SummaryModelContext&& context);
  ~SummaryModel();

  const TimeRange& time_range() const { return time_range_; }

  base::TimeDelta interval() const { return aggregate_filter_.interval; }
  void SetInterval(base::TimeDelta interval);

  const scada::NodeId& aggregate_type() const {
    return aggregate_filter_.aggregate_type;
  }
  void SetAggregateType(scada::NodeId aggregate_type);

  void SetParams(const TimeRange& time_range,
                 scada::AggregateFilter aggregate_filter);

  int AddColumn(std::string formula);
  void DeleteColumn(int index);
  int FindColumn(const scada::NodeId& node_id, int starting_index = 0) const;

  void Load(const WindowDefinition& definition);
  void Save(WindowDefinition& definition);

  ui::HeaderModel& row_model();
  ui::HeaderModel& column_model();

  TimedDataService& timed_data_service() { return timed_data_service_; }

  scada::DataValue GetDataValue(int row, int column) const;
  const TimedDataSpec& timed_data(int column) const;

  base::Time GetRowTime(int row) const;
  int GetRowForTime(base::Time time) const;

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

  // ExportModel
  virtual ExportData GetExportData() override;

  static bool IsCustomUnits(const scada::NodeId& aggregation_id);

 private:
  class Column;
  class ColumnModel;
  class RowModel;

  void OnCellChanged(int column, int row);
  void OnColumnChanged(int column);
  void OnColumnTitleChanged(int column);

  bool path_title_ = true;

  std::vector<std::unique_ptr<Column>> columns_;

  base::Time start_time_;
  base::Time end_time_;
  TimeRange time_range_;
  scada::AggregateFilter aggregate_filter_;

  size_t row_count_ = 0;

  std::unique_ptr<RowModel> row_model_;
  std::unique_ptr<ColumnModel> column_model_;
};
