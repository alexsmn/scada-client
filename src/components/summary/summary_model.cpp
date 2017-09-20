#include "components/summary/summary_model.h"

#include "base/strings/sys_string_conversions.h"
#include "base/format_time.h"
#include "window_definition.h"
#include "timed_data/timed_data_spec.h"
#include "ui/base/models/grid_range.h"

namespace {

base::Time AlignTime(base::Time time, base::TimeDelta interval, bool upper) {
  base::Time midnight = time.LocalMidnight();

  // Align to day if interval is larger or equal to day.
  if (interval.InDays() > 0) {
    return upper ? midnight + base::TimeDelta::FromDays(1) : midnight;
  }

  // For intraday intervals align to midnight.
  base::TimeDelta day_offset = time - midnight;
  time -= base::TimeDelta::FromMicroseconds(
      day_offset.InMicroseconds() % interval.InMicroseconds());
  if (upper)
    time += interval;
  return time;
}

size_t CalculateRowCount(base::Time start_time, base::Time end_time,
                         base::TimeDelta interval) {
  if (start_time >= end_time)
    return 0;
  if (interval.InMinutes() <= 0)
    return 0;

  base::TimeDelta delta = end_time - start_time;
  size_t count = static_cast<size_t>(delta / interval);
  return std::min(count, 1000u);
}

} // namespace

// SummaryModel::Cell ---------------------------------------------------------

class SummaryModel::Cell {
 public:
  const scada::DataValue& data_value() const { return tvq_; }

  void Clear() { tvq_ = scada::DataValue(); }

  bool Update(const scada::DataValue& data_value);

 private:
  scada::DataValue tvq_;
};

bool SummaryModel::Cell::Update(const scada::DataValue& data_value) {
  if (data_value.server_timestamp <= tvq_.server_timestamp)
    return false;

  tvq_ = data_value;
  return true;
}

// SummaryModel::Column -------------------------------------------------------

class SummaryModel::Column : public rt::TimedDataDelegate {
 public:
  Column(SummaryModel& model, int index, const std::string& formula);

  const base::string16& title() const { return title_; }

  int width() const { return width_; }
  void set_width(int width) { width_ = width; }

  const rt::TimedDataSpec& timed_data() const { return timed_data_; }

  void UpdateTimes();

  SummaryModel::Cell& GetCell(int row) { return cells_[row]; }

 private:
  void UpdateHistory();

  void OnTvq(const scada::DataValue& data_value);

  // rt::TimedDataDelegate
  virtual void OnTimedDataCorrections(rt::TimedDataSpec& spec, size_t count,
                                     const scada::DataValue* tvqs) override;
  virtual void OnTimedDataReady(rt::TimedDataSpec& spec) override;
  virtual void OnPropertyChanged(rt::TimedDataSpec& spec, const rt::PropertySet& properties) override;

  SummaryModel& model_;
  int index_;

  base::string16 title_;
  int width_;

  std::vector<SummaryModel::Cell> cells_;

  rt::TimedDataSpec timed_data_;
};

SummaryModel::Column::Column(SummaryModel& model, int index, const std::string& formula)
    : model_(model),
      index_(index),
      cells_(model.row_count_),
      width_(100) {
  timed_data_.set_delegate(this);
  timed_data_.SetFrom(model_.times().start_time);
  timed_data_.Connect(model_.timed_data_service(), formula);
  title_ = timed_data_.GetTitle();

  UpdateHistory();
}

void SummaryModel::Column::UpdateTimes() {
  cells_.resize(model_.row_count_);
  timed_data_.SetFrom(model_.times().start_time);
  UpdateHistory();
}

void SummaryModel::Column::UpdateHistory() {
  for (size_t i = 0; i < cells_.size(); ++i)
    cells_[i].Clear();

  // History.
  const rt::TimedVQMap* values = timed_data_.values();
  if (values) {
    rt::TimedVQMap::const_iterator i = values->lower_bound(model_.times().start_time);
    rt::TimedVQMap::const_iterator end = values->upper_bound(model_.times().end_time);
    for ( ; i != end; ++i) {
      int row = model_.GetRowForTime(i->first);
      if (row != -1) {
        cells_[row].Update(scada::DataValue(i->second.vq.value,
                                            i->second.vq.qualifier,
                                            i->first,
                                            i->second.collection_time));
      }
    }
  }

  // Current.
  int row = model_.GetRowForTime(timed_data_.current().source_timestamp);
  if (row != -1)
    cells_[row].Update(timed_data_.current());
}

void SummaryModel::Column::OnTvq(const scada::DataValue& data_value) {
  int row = model_.GetRowForTime(data_value.source_timestamp);
  if (row != -1) {
    if (cells_[row].Update(data_value))
      model_.OnCellChanged(index_, row);
  }
}

void SummaryModel::Column::OnTimedDataCorrections(
    rt::TimedDataSpec& spec, size_t count, const scada::DataValue* tvqs) {
  for (size_t i = 0; i < count; ++i)
    OnTvq(tvqs[i]);
}

void SummaryModel::Column::OnTimedDataReady(rt::TimedDataSpec& spec) {
  UpdateHistory();
  model_.OnColumnChanged(index_);
}

void SummaryModel::Column::OnPropertyChanged(
                                       rt::TimedDataSpec& spec,
                                       const rt::PropertySet& properties) {
  if (properties.is_current_changed())
    OnTvq(spec.current());

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

SummaryModel::RowModel::RowModel(SummaryModel& model)
    : model_(model) {
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

SummaryModel::SummaryModel(TimedDataService& timed_data_service)
    : timed_data_service_(timed_data_service),
      row_count_(0),
      row_model_(new RowModel(*this)),
      column_model_(new ColumnModel(*this)) {
}

ui::HeaderModel& SummaryModel::row_model() {
  return *row_model_;
}

ui::HeaderModel& SummaryModel::column_model() {
  return *column_model_;
}

void SummaryModel::SetTimes(const Times& times) {
  CHECK(times.start_time <= times.end_time);
  CHECK(times.interval.InMinutes() > 0);

  times_.start_time = AlignTime(times.start_time, times.interval, false);
  times_.end_time = AlignTime(times.end_time, times.interval, true);
  times_.interval = times.interval;
  row_count_ = CalculateRowCount(times_.start_time, times_.end_time,
                                 times_.interval);

  for (size_t i = 0; i < columns_.size(); ++i)
    columns_[i]->UpdateTimes();

  row_model_->NotifyModelChanged();
}

int SummaryModel::AddColumn(const std::string& formula) {
  int index = static_cast<int>(columns_.size());
  columns_.emplace_back(new Column(*this, index, formula));
  return index;
}

void SummaryModel::Load(const WindowDefinition& definition) {
  base::Time now = base::Time::Now();
  Times times = { now - base::TimeDelta::FromHours(5), now,
                  base::TimeDelta::FromMinutes(1) };
  SetTimes(times);

  size_t count = std::min(10u, definition.items.size());
  for (size_t i = 0; i < count; ++i) {
    const WindowItem& item = definition.items[i];
    if (item.name_is("Item")) {
      std::string path = item.GetString("path");
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
  c.text = column.timed_data().GetValueString(data_value.value, data_value.qualifier);

  if (!column.timed_data().ready())
    c.cell_color = SkColorSetRGB(227, 227, 227);
}

base::Time SummaryModel::GetRowTime(int row) const {
  DCHECK(row >= 0 && row < static_cast<int>(row_count_));
  return times_.start_time + times_.interval * row;
}

int SummaryModel::GetRowForTime(base::Time time) const {
  if ((time < times_.start_time) || (time >= times_.end_time))
    return -1;
  if (row_count_ == 0)
    return -1;

  base::TimeDelta delta = time - times_.start_time;
  int row = static_cast<int>(delta / times_.interval);
  DCHECK(row >= 0 && row < static_cast<int>(row_count_));
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
