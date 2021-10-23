#include "components/summary/summary_model.h"

#include "base/format_time.h"
#include "base/strings/sys_string_conversions.h"
#include "client_utils.h"
#include "common/aggregation.h"
#include "common/formula_util.h"
#include "components/summary/summary_model_util.h"
#include "node_service/node_service.h"
#include "node_service/node_util.h"
#include "timed_data/timed_data_spec.h"
#include "ui/base/models/grid_range.h"
#include "window_definition.h"
#include "window_definition_util.h"

#include "base/debug_util-inl.h"

// SummaryModel::Column -------------------------------------------------------

class SummaryModel::Column {
 public:
  Column(SummaryModel& model, int index, std::string formula);

  int width() const { return width_; }
  void set_width(int width) { width_ = width; }

  std::wstring GetTitle() const;
  scada::NodeId GetNodeId() const { return timed_data_.GetNode().node_id(); }

  void UpdateTimes();

  scada::DataValue GetDataValue(int row) const;
  bool IsReady(int row) const;

  const TimedDataSpec& timed_data() const { return timed_data_; }

 private:
  void OnTvq(const scada::DataValue& data_value);

  void OnTimedDataCorrections(size_t count, const scada::DataValue* tvqs);
  void OnTimedDataReady();
  void OnPropertyChanged(const PropertySet& properties);

  SummaryModel& model_;
  const int index_;
  const std::string formula_;

  int width_ = 0;

  TimedDataSpec timed_data_;
};

SummaryModel::Column::Column(SummaryModel& model,
                             int index,
                             std::string formula)
    : model_(model), index_(index), formula_{std::move(formula)}, width_(100) {
  timed_data_.correction_handler = [this](size_t count,
                                          const scada::DataValue* tvqs) {
    OnTimedDataCorrections(count, tvqs);
  };
  timed_data_.ready_handler = [this] { OnTimedDataReady(); };
  timed_data_.property_change_handler = [this](const PropertySet& properies) {
    OnPropertyChanged(properies);
  };
  UpdateTimes();
}

std::wstring SummaryModel::Column::GetTitle() const {
  if (model_.path_title_) {
    if (auto node = timed_data_.GetNode()) {
      auto title = GetFullDisplayName(node);
      if (!title.empty())
        return title;
    }
  }

  return timed_data_.GetTitle();
}

void SummaryModel::Column::UpdateTimes() {
  // Don't allow existing data to free too soon.
  auto timed_data_copy = timed_data_;

  timed_data_.Reset();
  timed_data_.SetAggregateFilter(model_.aggregate_filter_);
  timed_data_.SetRange({model_.start_time_, model_.end_time_});
  timed_data_.Connect(model_.timed_data_service(), formula_);
  model_.OnColumnChanged(index_);
}

void SummaryModel::Column::OnTvq(const scada::DataValue& data_value) {
  int row = model_.GetRowForTime(data_value.source_timestamp);
  if (row != -1)
    model_.OnCellChanged(index_, row);
}

void SummaryModel::Column::OnTimedDataCorrections(
    size_t count,
    const scada::DataValue* tvqs) {
  for (size_t i = 0; i < count; ++i)
    OnTvq(tvqs[i]);
}

void SummaryModel::Column::OnTimedDataReady() {
  model_.OnColumnChanged(index_);
}

void SummaryModel::Column::OnPropertyChanged(const PropertySet& properties) {
  if (properties.is_current_changed())
    OnTvq(timed_data_.current());

  if (properties.is_title_changed())
    model_.OnColumnTitleChanged(index_);

  if (properties.is_item_changed())
    model_.OnColumnChanged(index_);
}

scada::DataValue SummaryModel::Column::GetDataValue(int row) const {
  auto time = model_.GetRowTime(row);
  auto data_value = timed_data_.GetValueAt(time);
  if (data_value.is_null())
    return {};
  if (data_value.source_timestamp < time)
    return {};
  return data_value;
}

bool SummaryModel::Column::IsReady(int row) const {
  auto time = model_.GetRowTime(row);
#ifdef TIMED_DATA_RANGE_SUPPORT
  scada::DateTimeRange range{time, time + model_.interval()};
  return timed_data_.range_ready(range);
#else
  return time >= timed_data_.ready_from();
#endif
}

// SummaryModel::RowModel -----------------------------------------------------

class SummaryModel::RowModel : public ui::HeaderModel {
 public:
  explicit RowModel(SummaryModel& model);

  // ui::HeaderModel
  virtual int GetCount() const override { return model_.row_count_; }
  virtual int GetSize(int index) const override { return 18; }
  virtual std::wstring GetTitle(int index) const override;

 private:
  friend class SummaryModel;

  SummaryModel& model_;
};

SummaryModel::RowModel::RowModel(SummaryModel& model) : model_(model) {
  SetFixedSize(true);
}

std::wstring SummaryModel::RowModel::GetTitle(int index) const {
  base::Time time = model_.GetRowTime(index);
  return base::SysNativeMBToWide(
      FormatTime(time, TIME_FORMAT_DATE | TIME_FORMAT_TIME));
}

// SummaryModel::ColumnModel --------------------------------------------------

class SummaryModel::ColumnModel : public ui::HeaderModel {
 public:
  explicit ColumnModel(SummaryModel& model) : model_(model) {}

  // ui::HeaderModel
  virtual int GetCount() const override;
  virtual int GetSize(int index) const override;
  virtual void SetSize(int index, int new_size) override;
  virtual std::wstring GetTitle(int index) const override;
  virtual ui::TableColumn::Alignment GetAlignment(int index) const override;

 private:
  friend class SummaryModel;

  SummaryModel& model_;
};

int SummaryModel::ColumnModel::GetCount() const {
  return static_cast<int>(model_.columns_.size());
}

int SummaryModel::ColumnModel::GetSize(int index) const {
  return model_.columns_[index]->width();
}

void SummaryModel::ColumnModel::SetSize(int index, int new_size) {
  SummaryModel::Column& column = *model_.columns_[index];
  if (column.width() != new_size) {
    column.set_width(new_size);
    NotifySizeChanged(index);
  }
}

std::wstring SummaryModel::ColumnModel::GetTitle(int index) const {
  return model_.columns_[index]->GetTitle();
}

ui::TableColumn::Alignment SummaryModel::ColumnModel::GetAlignment(
    int index) const {
  return ui::TableColumn::RIGHT;
}

// SummaryModel ---------------------------------------------------------------

SummaryModel::SummaryModel(SummaryModelContext&& context)
    : SummaryModelContext{std::move(context)},
      row_model_(new RowModel(*this)),
      column_model_(new ColumnModel(*this)) {}

SummaryModel::~SummaryModel() = default;

ui::HeaderModel& SummaryModel::row_model() {
  return *row_model_;
}

ui::HeaderModel& SummaryModel::column_model() {
  return *column_model_;
}

int SummaryModel::AddColumn(std::string formula) {
  int index = static_cast<int>(columns_.size());
  auto& column = columns_.emplace_back(
      std::make_unique<Column>(*this, index, std::move(formula)));
  column_model_->NotifyModelChanged();

  auto node_id = column->GetNodeId();
  if (!node_id.is_null())
    NotifyContainedItemChanged(node_id, true);

  return index;
}

void SummaryModel::DeleteColumn(int index) {
  auto node_id = columns_[index]->GetNodeId();

  columns_.erase(columns_.begin() + index);
  column_model_->NotifyModelChanged();

  if (!node_id.is_null() && FindColumn(node_id) == -1)
    NotifyContainedItemChanged(node_id, false);
}

int SummaryModel::FindColumn(const scada::NodeId& node_id,
                             int starting_index) const {
  for (int i = starting_index; i < static_cast<int>(columns_.size()); ++i) {
    if (columns_[i]->GetNodeId() == node_id)
      return i;
  }
  return -1;
}

void SummaryModel::Load(const WindowDefinition& definition) {
  const auto time_range =
      RestoreTimeRange(definition).value_or(TimeRange::Type::Day);
  const auto interval = definition.Get<scada::Duration>("Interval")
                            .value_or(scada::Duration::FromHours(1));
  const scada::NodeId aggregate_type =
      definition.Get<scada::NodeId>("AggregateType")
          .value_or(scada::id::AggregateFunction_End);

  SetParams(time_range,
            scada::AggregateFilter{scada::GetLocalAggregateStartTime(),
                                   interval, aggregate_type});

  size_t count = std::min(kMaxColumnCount, definition.items.size());
  for (size_t i = 0; i < count; ++i) {
    const WindowItem& item = definition.items[i];
    if (item.name_is("Item")) {
      auto path = item.GetString("path");
      int width = item.GetInt("width", 100);
      int index = AddColumn(std::string{path});
      columns_[index]->set_width(width);
    }
  }

  NotifyModelChanged();
  row_model_->NotifyModelChanged();
  column_model_->NotifyModelChanged();
}

void SummaryModel::Save(WindowDefinition& definition) {
  for (size_t i = 0; i < columns_.size(); ++i) {
    const Column& column = *columns_[i];
    WindowItem& item = definition.AddItem("Item");
    item.SetString("path", column.timed_data().formula());
    item.SetInt("width", column.width());
  }

  SaveTimeRange(definition, GetTimeRange());
  definition.Set("Interval", interval());
  definition.Set("AggregateType", aggregate_type());
}

void SummaryModel::GetCell(ui::GridCell& cell) {
  Column& column = *columns_[cell.column];

  const scada::DataValue& data_value = column.GetDataValue(cell.row);

  if (IsCustomUnits(aggregate_filter_.aggregate_type)) {
    cell.text = ToString16(data_value.value);
  } else {
    cell.text = column.timed_data().GetValueString(data_value.value,
                                                   data_value.qualifier);
  }

  if (!column.IsReady(cell.row))
    cell.cell_color = SkColorSetRGB(227, 227, 227);
}

base::Time SummaryModel::GetRowTime(int row) const {
  assert(row >= 0 && row < static_cast<int>(row_count_));
  assert(!start_time_.is_null());
  assert(!aggregate_filter_.interval.is_zero());
  return start_time_ + aggregate_filter_.interval * row;
}

int SummaryModel::GetRowForTime(base::Time time) const {
  assert(!start_time_.is_null());
  assert(!end_time_.is_null());
  assert(start_time_ <= end_time_);
  assert(!aggregate_filter_.interval.is_zero());

  // |end_time_| defines start of the last interval.
  if (time < start_time_ || time >= end_time_)
    return -1;
  if (row_count_ == 0)
    return -1;

  base::TimeDelta delta = time - start_time_;
  int row = static_cast<int>(delta / aggregate_filter_.interval);
  assert(row >= 0 && row < static_cast<int>(row_count_));
  return row;
}

void SummaryModel::OnCellChanged(int column, int row) {
  NotifyRangeChanged(ui::GridRange::Cell(row, column));
}

void SummaryModel::OnColumnChanged(int column) {
  NotifyRangeChanged(ui::GridRange::Column(column));
}

void SummaryModel::OnColumnTitleChanged(int column) {
  column_model_->NotifyModelChanged();
}

void SummaryModel::AddContainedItem(const scada::NodeId& node_id,
                                    unsigned flags) {
  // TOOD: Capture by weak pointer.
  ExpandGroupItemIds(node_service_.GetNode(node_id))
      .then([this](const NodeIdSet& child_ids) {
        for (const auto& child_id : child_ids)
          AddColumn(MakeNodeIdFormula(child_id));
      });
}

void SummaryModel::RemoveContainedItem(const scada::NodeId& node_id) {
  for (int index = FindColumn(node_id); index != -1;
       index = FindColumn(node_id, index)) {
    DeleteColumn(index);
  }
}

NodeIdSet SummaryModel::GetContainedItems() const {
  NodeIdSet node_ids;
  for (auto& column : columns_) {
    auto node_id = column->GetNodeId();
    if (!node_id.is_null())
      node_ids.emplace(node_id);
  }
  return node_ids;
}

TimeRange SummaryModel::GetTimeRange() const {
  return time_range_;
}

void SummaryModel::SetTimeRange(const TimeRange& time_range) {
  SetParams(time_range, aggregate_filter_);
}

void SummaryModel::SetAggregateType(scada::NodeId aggregate_type) {
  auto new_filter = aggregate_filter_;
  new_filter.aggregate_type = std::move(aggregate_type);
  SetParams(time_range_, std::move(new_filter));
}

void SummaryModel::SetInterval(base::TimeDelta interval) {
  auto new_filter = aggregate_filter_;
  new_filter.interval = interval;
  SetParams(time_range_, std::move(new_filter));
}

void SummaryModel::SetParams(const TimeRange& time_range,
                             scada::AggregateFilter aggregate_filter) {
  assert(!aggregate_filter.is_null());

  auto params =
      CalculateSummaryModelParams(time_range, aggregate_filter.interval);

  time_range_ = time_range;
  aggregate_filter_ = std::move(aggregate_filter);
  start_time_ = params.start_time;
  end_time_ = params.end_time;
  row_count_ = params.row_count;

  LOG_INFO(logger_) << "Calculated params"
                    << LOG_TAG("StartTime", ToString(start_time_))
                    << LOG_TAG("EndTime", ToString(end_time_))
                    << LOG_TAG("RowCount", row_count_)
                    << LOG_TAG("TimeRange", ToString(time_range_))
                    << LOG_TAG("AggregateFilter", ToString(aggregate_filter_));

  for (size_t i = 0; i < columns_.size(); ++i)
    columns_[i]->UpdateTimes();

  NotifyModelChanged();
}

scada::DataValue SummaryModel::GetDataValue(int row, int column) const {
  Column& col = *columns_[column];
  return col.GetDataValue(row);
}

const TimedDataSpec& SummaryModel::timed_data(int column) const {
  return columns_[column]->timed_data();
}

// static
bool SummaryModel::IsCustomUnits(const scada::NodeId& aggregation_id) {
  return aggregation_id == scada::id::AggregateFunction_Count;
}

ExportModel::ExportData SummaryModel::GetExportData() {
  return GridExportData{
      ui::TableColumn{-1, L"Время", 100, ui::TableColumn::LEFT,
                      ui::TableColumn::DataType::General},
      *this, row_model(), column_model()};
}
