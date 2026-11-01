#pragma once

#include "base/any_executor.h"
#include "base/lifetime.h"

#include "aui/models/grid_model.h"
#include "base/boost_log.h"
#include "base/time/time.h"
#include "common/node_state.h"
#include "controller/contents_model.h"
#include "controller/time_model.h"
#include "export/export_model.h"
#include "node_service/node_ref.h"
#include "scada/aggregate_filter.h"
#include "scada/date_time.h"

#include <functional>
#include <memory>
#include <optional>

namespace scada {
class DataValue;
}

class NodeService;
class TimedDataService;
class TimedDataSpec;
class WindowDefinition;

struct SummaryModelContext {
  AnyExecutor executor_;
  // The node service is only used when adding contained items.
  NodeService& node_service_;
  TimedDataService& timed_data_service_;
};

class SummaryModel : private SummaryModelContext,
                     public scada::aui::GridModel,
                     public ContentsModel,
                     public TimeModel,
                     public ExportModel {
 public:
  explicit SummaryModel(SummaryModelContext&& context);
  ~SummaryModel();

  const scada::RelativeTimeRange& time_range() const SCADA_LIFETIME_BOUND {
    return time_range_;
  }

  scada::Duration interval() const { return aggregate_filter_.interval; }
  void SetInterval(scada::Duration interval);

  const scada::NodeId& aggregate_type() const {
    return aggregate_filter_.aggregate_type;
  }
  void SetAggregateType(scada::NodeId aggregate_type);

  void SetParams(const scada::RelativeTimeRange& time_range,
                 scada::AggregateFilter aggregate_filter);

  // Adds a column and returns its index, or -1 when the summary already holds
  // kMaxColumnCount columns; the refusal is counted in dropped_column_count().
  int AddColumn(std::string formula);
  void DeleteColumn(int index);
  int FindColumn(const scada::NodeId& node_id, int starting_index = 0) const;

  void Load(const WindowDefinition& definition);
  void Save(WindowDefinition& definition);

  scada::aui::HeaderModel& row_model();
  scada::aui::HeaderModel& column_model();

  TimedDataService& timed_data_service() { return timed_data_service_; }

  scada::DataValue GetDataValue(int row, int column) const;
  const TimedDataSpec& timed_data(int column) const;

  scada::Time GetRowTime(int row) const;

  // When the period needs more rows than the summary holds, the time its last
  // row ends at; nullopt when the whole period fits. The cap used to cut the
  // period silently (backlog 871).
  std::optional<scada::Time> truncated_at() const {
    return truncated_ ? std::optional{end_time_} : std::nullopt;
  }

  // How many columns the column cap has refused since the summary was opened:
  // items added past it, or saved items a reopen could not restore. Both used
  // to vanish without a word (backlog 962).
  size_t dropped_column_count() const { return dropped_column_count_; }

  // Called after the period, the interval or the dropped-column count
  // changes, so a view can show or hide its notice.
  std::function<void()> notice_changed_handler;
  int GetRowForTime(scada::Time time) const;

  // aui::GridModel
  virtual void GetCell(scada::aui::GridCell& cell) override;

  // ContentsModel
  virtual void AddContainedItem(const scada::NodeId& node_id,
                                unsigned flags) override;
  virtual void RemoveContainedItem(const scada::NodeId& node_id) override;
  virtual NodeIdSet GetContainedItems() const override;

  // TimeModel
  virtual scada::RelativeTimeRange GetTimeRange() const override;
  virtual void SetTimeRange(
      const scada::RelativeTimeRange& time_range) override;

  // ExportModel
  virtual ExportData GetExportData() override;
  virtual std::u16string GetExportDescription() override;

  static bool IsCustomUnits(const scada::NodeId& aggregation_id);

  // The sentence a view and a printout show when the row cap cut the period.
  static std::u16string TruncationNotice(scada::Time cut);

  // The sentence a view shows when the column cap refused |dropped| items.
  static std::u16string ColumnLimitNotice(size_t dropped);

  // Everything the view's notice says now, or empty when it has nothing.
  std::u16string GetNotice() const;

 private:
  class Column;
  class ColumnModel;
  class RowModel;

  void OnCellChanged(int column, int row);
  void OnColumnChanged(int column);
  void OnColumnTitleChanged(int column);

  // Whether `index` names a live column. The header models are queried by
  // section number by a `QHeaderView` that may still be holding a count from
  // before a `DeleteColumn`, so they ask this before indexing `columns_`.
  bool IsColumnIndex(int index) const {
    return index >= 0 && index < static_cast<int>(columns_.size());
  }

  BoostLogger logger_{LOG_NAME("SummaryModel")};

  bool path_title_ = true;

  std::vector<std::unique_ptr<Column>> columns_;

  scada::Time start_time_ = scada::kNullTime;
  bool truncated_ = false;
  size_t dropped_column_count_ = 0;
  // |end_time_| defines start of the last interval.
  scada::Time end_time_ = scada::kNullTime;
  scada::RelativeTimeRange time_range_;
  scada::AggregateFilter aggregate_filter_;

  size_t row_count_ = 0;

  std::unique_ptr<RowModel> row_model_;
  std::unique_ptr<ColumnModel> column_model_;
};
