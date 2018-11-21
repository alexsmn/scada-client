#include "components/summary/summary_model.h"

#include "base/format_time.h"
#include "base/strings/sys_string_conversions.h"
#include "timed_data/timed_data_spec.h"
#include "ui/base/models/grid_range.h"
#include "window_definition.h"

namespace {

const unsigned kMaxRowCount = 10000;

base::Time AlignTime(base::Time time, base::TimeDelta interval, bool upper) {
  base::Time midnight = time.LocalMidnight();

  // Align to day if interval is larger or equal to day.
  if (interval.InDays() > 0) {
    return upper ? midnight + base::TimeDelta::FromDays(1) : midnight;
  }

  // For intraday intervals align to midnight.
  base::TimeDelta day_offset = time - midnight;
  time -= base::TimeDelta::FromMicroseconds(day_offset.InMicroseconds() %
                                            interval.InMicroseconds());
  if (upper)
    time += interval;
  return time;
}

}  // namespace

// SummaryModel::Cell ---------------------------------------------------------

class SummaryModel::Cell {
 public:
  const scada::DataValue& data_value() const { return tvq_; }

  void Clear() { tvq_ = scada::DataValue(); }

  bool Update(AggregationFunction aggregation_function,
              const scada::DataValue& data_value);

 private:
  scada::DataValue tvq_;
  size_t count_ = 0;
  std::optional<double> double_value_;
};

bool SummaryModel::Cell::Update(AggregationFunction aggregation_function,
                                const scada::DataValue& data_value) {
  double double_value = 0.0;

  switch (aggregation_function) {
    case AggregationFunction::Last:
      if (!scada::IsUpdate(tvq_, data_value))
        return false;
      tvq_ = data_value;
      ++count_;
      return true;

    case AggregationFunction::Count:
      ++count_;
      tvq_ = data_value;
      tvq_.value = count_;
      return true;

    case AggregationFunction::Min:
      if (!data_value.value.get(double_value))
        return false;
      if (double_value_ && *double_value_ <= double_value)
        return false;
      double_value_ = double_value;
      tvq_ = data_value;
      return true;

    case AggregationFunction::Max:
      if (!data_value.value.get(double_value))
        return false;
      if (double_value_ && *double_value_ >= double_value)
        return false;
      double_value_ = double_value;
      tvq_ = data_value;
      return true;

    case AggregationFunction::Avg:
      if (!data_value.value.get(double_value))
        return false;
      if (double_value_)
        *double_value_ += double_value;
      else
        double_value_ = double_value;
      ++count_;
      tvq_ = data_value;
      tvq_.value = *double_value_ / count_;
      return true;

    case AggregationFunction::Sum:
      if (!data_value.value.get(double_value))
        return false;
      if (double_value_)
        *double_value_ += double_value;
      else
        double_value_ = double_value;
      tvq_ = data_value;
      tvq_.value = *double_value_;
      return true;

    default:
      return false;
  }
}

// SummaryModel::Column -------------------------------------------------------

class SummaryModel::Column {
 public:
  Column(SummaryModel& model, int index, base::StringPiece formula);

  const base::string16& title() const { return title_; }

  int width() const { return width_; }
  void set_width(int width) { width_ = width; }

  const rt::TimedDataSpec& timed_data() const { return timed_data_; }

  void UpdateTimes();

  SummaryModel::Cell& GetCell(int row) { return cells_[row]; }

 private:
  void UpdateHistory();

  void OnTvq(const scada::DataValue& data_value);

  void OnTimedDataCorrections(size_t count, const scada::DataValue* tvqs);
  void OnTimedDataReady();
  void OnPropertyChanged(const rt::PropertySet& properties);

  SummaryModel& model_;
  int index_;

  base::string16 title_;
  int width_;

  std::vector<SummaryModel::Cell> cells_;

  rt::TimedDataSpec timed_data_;
};

SummaryModel::Column::Column(SummaryModel& model,
                             int index,
                             base::StringPiece formula)
    : model_(model), index_(index), cells_(model.row_count_), width_(100) {
  timed_data_.correction_handler = [this](size_t count,
                                          const scada::DataValue* tvqs) {
    OnTimedDataCorrections(count, tvqs);
  };
  timed_data_.ready_handler = [this] { OnTimedDataReady(); };
  timed_data_.property_change_handler =
      [this](const rt::PropertySet& properies) {
        OnPropertyChanged(properies);
      };
  timed_data_.SetFrom(model_.start_time_);
  timed_data_.Connect(model.timed_data_service(), formula);
  title_ = timed_data_.GetTitle();

  UpdateHistory();
}

void SummaryModel::Column::UpdateTimes() {
  cells_.resize(model_.row_count_);
  timed_data_.SetFrom(model_.start_time_);
  UpdateHistory();
}

void SummaryModel::Column::UpdateHistory() {
  for (size_t i = 0; i < cells_.size(); ++i)
    cells_[i].Clear();

  // History.
  if (const rt::DataValues* values = timed_data_.values()) {
    auto i = rt::LowerBound(*values, model_.start_time_);
    auto end = rt::UpperBound(*values, model_.end_time_);
    for (; i != end; ++i) {
      int row = model_.GetRowForTime(i->source_timestamp);
      if (row != -1)
        cells_[row].Update(model_.aggregation_function_, *i);
    }
  }

  // Current.
  int row = model_.GetRowForTime(timed_data_.current().source_timestamp);
  if (row != -1)
    cells_[row].Update(model_.aggregation_function_, timed_data_.current());
}

void SummaryModel::Column::OnTvq(const scada::DataValue& data_value) {
  int row = model_.GetRowForTime(data_value.source_timestamp);
  if (row != -1) {
    if (cells_[row].Update(model_.aggregation_function_, data_value))
      model_.OnCellChanged(index_, row);
  }
}

void SummaryModel::Column::OnTimedDataCorrections(
    size_t count,
    const scada::DataValue* tvqs) {
  for (size_t i = 0; i < count; ++i)
    OnTvq(tvqs[i]);
}

void SummaryModel::Column::OnTimedDataReady() {
  UpdateHistory();
  model_.OnColumnChanged(index_);
}

void SummaryModel::Column::OnPropertyChanged(
    const rt::PropertySet& properties) {
  if (properties.is_current_changed())
    OnTvq(timed_data_.current());

  if (properties.is_title_changed()) {
    title_ = timed_data_.GetTitle();
    model_.OnColumnTitleChanged(index_);
  }

  if (properties.is_item_changed())
    model_.OnColumnChanged(index_);
}

// SummaryModel::RowModel -----------------------------------------------------

class SummaryModel::RowModel : public ui::HeaderModel {
 public:
  explicit RowModel(SummaryModel& model);

  // ui::HeaderModel
  virtual int GetCount() override { return model_.row_count_; }
  virtual int GetSize(int index) override { return 18; }
  virtual base::string16 GetTitle(int index) override;

 private:
  friend class SummaryModel;

  SummaryModel& model_;
};

SummaryModel::RowModel::RowModel(SummaryModel& model) : model_(model) {
  SetFixedSize(true);
}

base::string16 SummaryModel::RowModel::GetTitle(int index) {
  base::Time time = model_.GetRowTime(index);
  return base::SysNativeMBToWide(
      FormatTime(time, TIME_FORMAT_DATE | TIME_FORMAT_TIME));
}

// SummaryModel::ColumnModel --------------------------------------------------

class SummaryModel::ColumnModel : public ui::HeaderModel {
 public:
  explicit ColumnModel(SummaryModel& model) : model_(model) {}

  // ui::HeaderModel
  virtual int GetCount() override;
  virtual int GetSize(int index) override;
  virtual void SetSize(int index, int new_size) override;
  virtual base::string16 GetTitle(int index) override;
  virtual ui::TableColumn::Alignment GetAlignment(int index) override;

 private:
  friend class SummaryModel;

  SummaryModel& model_;
};

int SummaryModel::ColumnModel::GetCount() {
  return static_cast<int>(model_.columns_.size());
}

int SummaryModel::ColumnModel::GetSize(int index) {
  return model_.columns_[index]->width();
}

void SummaryModel::ColumnModel::SetSize(int index, int new_size) {
  SummaryModel::Column& column = *model_.columns_[index];
  if (column.width() != new_size) {
    column.set_width(new_size);
    NotifySizeChanged(index);
  }
}

base::string16 SummaryModel::ColumnModel::GetTitle(int index) {
  return model_.columns_[index]->title();
}

ui::TableColumn::Alignment SummaryModel::ColumnModel::GetAlignment(int index) {
  return ui::TableColumn::RIGHT;
}

// SummaryModel ---------------------------------------------------------------

SummaryModel::SummaryModel(SummaryModelContext&& context)
    : SummaryModelContext{std::move(context)},
      row_model_(new RowModel(*this)),
      column_model_(new ColumnModel(*this)) {}

ui::HeaderModel& SummaryModel::row_model() {
  return *row_model_;
}

ui::HeaderModel& SummaryModel::column_model() {
  return *column_model_;
}

int SummaryModel::AddColumn(base::StringPiece formula) {
  int index = static_cast<int>(columns_.size());
  columns_.emplace_back(new Column(*this, index, formula));
  return index;
}

void SummaryModel::Load(const WindowDefinition& definition) {
  // TODO: Load time range and interval.
  SetParams(TimeRange{ID_TIME_RANGE_DAY}, base::TimeDelta::FromHours(1),
            AggregationFunction::Last);

  size_t count = std::min(10u, definition.items.size());
  for (size_t i = 0; i < count; ++i) {
    const WindowItem& item = definition.items[i];
    if (item.name_is("Item")) {
      auto path = item.GetString("path");
      int width = item.GetInt("width", 100);
      int index = AddColumn(path);
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
}

void SummaryModel::GetCell(ui::GridCell& c) {
  Column& column = *columns_[c.column];
  Cell& cell = column.GetCell(c.row);

  const scada::DataValue& data_value = cell.data_value();
  if (IsCustomUnits(aggregation_function_)) {
    c.text = ToString16(data_value.value);
  } else {
    c.text = column.timed_data().GetValueString(data_value.value,
                                                data_value.qualifier);
  }

  if (!column.timed_data().ready())
    c.cell_color = SkColorSetRGB(227, 227, 227);
}

base::Time SummaryModel::GetRowTime(int row) const {
  assert(row >= 0 && row < static_cast<int>(row_count_));
  assert(!start_time_.is_null());
  assert(!interval_.is_zero());
  return start_time_ + interval_ * row;
}

int SummaryModel::GetRowForTime(base::Time time) const {
  assert(!start_time_.is_null());
  assert(!end_time_.is_null());
  assert(start_time_ <= end_time_);
  assert(!interval_.is_zero());

  if (time < start_time_ || time >= end_time_)
    return -1;
  if (row_count_ == 0)
    return -1;

  base::TimeDelta delta = time - start_time_;
  int row = static_cast<int>(delta / interval_);
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

TimeRange SummaryModel::GetTimeRange() const {
  return time_range_;
}

void SummaryModel::SetTimeRange(const TimeRange& time_range) {
  SetParams(time_range, interval_, aggregation_function_);
}

void SummaryModel::SetAggregationFunction(
    AggregationFunction aggregation_function) {
  SetParams(time_range_, interval_, aggregation_function);
}

void SummaryModel::SetInterval(base::TimeDelta interval) {
  SetParams(time_range_, interval, aggregation_function_);
}

void SummaryModel::SetParams(const TimeRange& time_range,
                             base::TimeDelta interval,
                             AggregationFunction aggregation_function) {
  assert(!interval.is_zero());

  auto [start_time, end_time] = GetTimeRangeBounds(time_range);

  // Can update |interval_|.
  auto delta = end_time - start_time;
  int64_t row_count = std::max(base::TimeDelta(),
                               delta - base::TimeDelta::FromMicroseconds(1)) /
                          interval +
                      1;
  row_count = std::min(row_count, static_cast<int64_t>(kMaxRowCount));

  time_range_ = time_range;
  start_time_ = start_time;
  end_time_ = start_time_ + interval * row_count;
  interval_ = interval;
  row_count_ = row_count;
  aggregation_function_ = aggregation_function;

  assert(!interval_.is_zero());
  assert(!start_time_.is_null());
  assert(!end_time_.is_null());
  assert(start_time_ <= end_time_);
  assert(row_count_ <= kMaxRowCount);

  for (size_t i = 0; i < columns_.size(); ++i)
    columns_[i]->UpdateTimes();

  NotifyModelChanged();
}

// static
bool SummaryModel::IsCustomUnits(AggregationFunction aggregation_function) {
  return aggregation_function == AggregationFunction::Count;
}
