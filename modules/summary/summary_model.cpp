#include "modules/summary/summary_model.h"

#include "aui/models/grid_range.h"
#include "aui/translation.h"
#include "base/awaitable.h"
#include "base/check.h"
#include "base/format_time.h"
#include "base/u16format.h"
#include "base/utf_convert.h"
#include "common/aggregation.h"
#include "common/formula_util.h"
#include "model/data_items_node_ids.h"
#include "modules/summary/summary_model_util.h"
#include "net/net_executor_adapter.h"
#include "node_service/node_awaitable.h"
#include "node_service/node_service.h"
#include "node_service/node_util.h"
#include "profile/window_definition.h"
#include "profile/window_definition_util.h"
#include "services/server_compatibility.h"
#include "timed_data/timed_data_property.h"
#include "timed_data/timed_data_spec.h"
#include "ui/common/client_utils.h"

#include "base/utf_convert.h"

#include "base/debug_util.h"

#include <algorithm>

namespace {

Awaitable<void> AddContainedItemAsync(SummaryModel& model,
                                      AnyExecutor executor,
                                      NodeRef node) {
  auto child_ids = co_await ExpandGroupItemIdsAsync(std::move(executor), node);
  for (const auto& child_id : child_ids) {
    model.AddColumn(MakeNodeIdFormula(child_id));
  }
  co_return;
}

}  // namespace

// SummaryModel::Column -------------------------------------------------------

class SummaryModel::Column {
 public:
  // A `comparison` column reads the same formula comparison_shift() earlier.
  Column(SummaryModel& model, int index, std::string formula, bool comparison);

  const std::string& formula() const { return formula_; }
  bool comparison() const { return comparison_; }
  void set_index(int index) { index_ = index; }

  int width() const { return width_; }
  void set_width(int width) { width_ = width; }

  std::u16string GetTitle() const;
  scada::NodeId GetNodeId() const { return timed_data_.node_id(); }

  void UpdateTimes();

  scada::DataValue GetDataValue(int row) const;
  bool IsReady(int row) const;

  const TimedDataSpec& timed_data() const { return timed_data_; }

 private:
  // The Last aggregate's value for `row`, or an empty value when the interval
  // has none. Row -1 is the lead-in interval before the period.
  scada::DataValue GetLastValue(int row) const;
  // The consumption of `row` (ComputeConsumption), computed for every row at
  // once and kept until the data changes.
  scada::DataValue GetConsumptionValue(int row) const;

  void OnTvq(const scada::DataValue& data_value);

  void OnTimedDataUpdates(std::span<const scada::DataValue> values);
  void OnTimedDataReady();
  void OnPropertyChanged(const PropertySet& properties);

  // The row's own interval start, moved back for a comparison column.
  scada::Time GetSourceTime(int row) const;

  SummaryModel& model_;
  int index_;
  const std::string formula_;
  const bool comparison_;

  int width_ = 0;

  TimedDataSpec timed_data_;

  // ComputeConsumption over the column, reset whenever its data changes.
  mutable std::optional<std::vector<std::optional<double>>> consumption_;
};

SummaryModel::Column::Column(SummaryModel& model,
                             int index,
                             std::string formula,
                             bool comparison)
    : model_(model),
      index_(index),
      formula_{std::move(formula)},
      comparison_{comparison},
      width_(100) {
  timed_data_.update_handler =
      [this](std::span<const scada::DataValue> values) {
        OnTimedDataUpdates(values);
      };
  timed_data_.ready_handler = [this] { OnTimedDataReady(); };
  timed_data_.property_change_handler = [this](const PropertySet& properies) {
    OnPropertyChanged(properies);
  };
  UpdateTimes();
}

std::u16string SummaryModel::Column::GetTitle() const {
  std::u16string title;
  if (model_.path_title_) {
    if (auto node = timed_data_.node())
      title = GetFullDisplayName(node);
  }
  if (title.empty())
    title = timed_data_.GetTitle().text;

  if (!comparison_)
    return title;

  // Names the window it reads, so a printout of the columns side by side
  // says which is which.
  const scada::Time start = model_.start_time_ - model_.comparison_shift();
  return u16format(L"{} ({} {})", title, Translate("from"),
                   UtfConvert<char16_t>(
                       FormatTime(start, TIME_FORMAT_DATE | TIME_FORMAT_TIME)));
}

scada::Time SummaryModel::Column::GetSourceTime(int row) const {
  // GetRowTime() checks its row; the lead-in row lies before the first.
  const scada::Time time = row == -1 ? model_.start_time_ - model_.interval()
                                     : model_.GetRowTime(row);
  return comparison_ ? time - model_.comparison_shift() : time;
}

void SummaryModel::Column::UpdateTimes() {
  // Don't allow existing data to free too soon.
  auto timed_data_copy = timed_data_;

  timed_data_.Reset();
  consumption_.reset();
  timed_data_.SetAggregateFilter(model_.aggregate_filter_);
  // Consumption measures the first row from the interval before it.
  const scada::Duration shift =
      comparison_ ? model_.comparison_shift() : scada::Duration::zero();
  const scada::Time start_time = model_.consumption_
                                     ? model_.start_time_ - model_.interval()
                                     : model_.start_time_;
  timed_data_.SetRange({start_time - shift, model_.end_time_ - shift});
  timed_data_.Connect(model_.timed_data_service(), formula_);
  model_.OnColumnChanged(index_);
}

void SummaryModel::Column::OnTvq(const scada::DataValue& data_value) {
  if (consumption_) {
    // A reading changes its own row and every row measured from it, which
    // can be any row below.
    consumption_.reset();
    model_.OnColumnChanged(index_);
    return;
  }
  int row = model_.GetRowForTime(comparison_ ? data_value.source_timestamp +
                                                   model_.comparison_shift()
                                             : data_value.source_timestamp);
  if (row != -1)
    model_.OnCellChanged(index_, row);
}

void SummaryModel::Column::OnTimedDataUpdates(
    std::span<const scada::DataValue> values) {
  for (const auto& value : values) {
    OnTvq(value);
  }
}

void SummaryModel::Column::OnTimedDataReady() {
  consumption_.reset();
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
  if (model_.consumption_)
    return GetConsumptionValue(row);
  return GetLastValue(row);
}

scada::DataValue SummaryModel::Column::GetLastValue(int row) const {
  auto time = GetSourceTime(row);
  const auto* value = timed_data_.GetValueAt(time);
  if (!value || value->source_timestamp < time) {
    return {};
  }
  return *value;
}

scada::DataValue SummaryModel::Column::GetConsumptionValue(int row) const {
  if (!consumption_) {
    const auto reading = [this](int r) -> std::optional<double> {
      double value = 0;
      scada::DataValue data_value = GetLastValue(r);
      if (data_value.is_null() || !data_value.value.get(value))
        return std::nullopt;
      return value;
    };
    std::vector<std::optional<double>> readings;
    readings.reserve(model_.row_count_);
    for (int r = 0; r < static_cast<int>(model_.row_count_); ++r)
      readings.push_back(reading(r));
    consumption_ = ComputeConsumption(reading(-1), readings);
  }

  const std::optional<double>& value = (*consumption_)[row];
  if (!value)
    return {};
  const scada::Time time = model_.GetRowTime(row);
  return scada::DataValue{*value, {}, time, time};
}

bool SummaryModel::Column::IsReady(int row) const {
  auto time = GetSourceTime(row);
  return timed_data_.range_ready({time, time + model_.interval()});
}

// SummaryModel::RowModel -----------------------------------------------------

class SummaryModel::RowModel : public scada::aui::HeaderModel {
 public:
  explicit RowModel(SummaryModel& model);

  // aui::HeaderModel
  virtual int GetCount() const override { return model_.row_count_; }
  virtual int GetSize(int index) const override { return 18; }
  virtual std::u16string GetTitle(int index) const override;

 private:
  friend class SummaryModel;

  SummaryModel& model_;
};

SummaryModel::RowModel::RowModel(SummaryModel& model) : model_(model) {
  SetFixedSize(true);
}

std::u16string SummaryModel::RowModel::GetTitle(int index) const {
  scada::Time time = model_.GetRowTime(index);
  return UtfConvert<char16_t>(
      FormatTime(time, TIME_FORMAT_DATE | TIME_FORMAT_TIME));
}

// SummaryModel::ColumnModel --------------------------------------------------

class SummaryModel::ColumnModel : public scada::aui::HeaderModel {
 public:
  explicit ColumnModel(SummaryModel& model) : model_(model) {}

  // aui::HeaderModel
  virtual int GetCount() const override;
  virtual int GetSize(int index) const override;
  virtual void SetSize(int index, int new_size) override;
  virtual std::u16string GetTitle(int index) const override;
  virtual scada::aui::TableColumn::Alignment GetAlignment(
      int index) const override;

 private:
  friend class SummaryModel;

  SummaryModel& model_;
};

int SummaryModel::ColumnModel::GetCount() const {
  return static_cast<int>(model_.columns_.size());
}

// A header is queried by section number, and `QHeaderView` repaints from the
// count it last heard about -- which outlives a `DeleteColumn` until the
// structural change reaches it. So every accessor here answers a default out
// of range rather than indexing a shrunk vector, the same way
// `ColumnHeaderModel` does.
int SummaryModel::ColumnModel::GetSize(int index) const {
  if (!model_.IsColumnIndex(index))
    return 0;
  return model_.columns_[index]->width();
}

void SummaryModel::ColumnModel::SetSize(int index, int new_size) {
  if (!model_.IsColumnIndex(index))
    return;
  SummaryModel::Column& column = *model_.columns_[index];
  if (column.width() != new_size) {
    column.set_width(new_size);
    NotifySizeChanged(index);
  }
}

std::u16string SummaryModel::ColumnModel::GetTitle(int index) const {
  if (!model_.IsColumnIndex(index))
    return {};
  return model_.columns_[index]->GetTitle();
}

scada::aui::TableColumn::Alignment SummaryModel::ColumnModel::GetAlignment(
    int index) const {
  return scada::aui::TableColumn::RIGHT;
}

// SummaryModel ---------------------------------------------------------------

SummaryModel::SummaryModel(SummaryModelContext&& context)
    : SummaryModelContext{std::move(context)},
      row_model_(new RowModel(*this)),
      column_model_(new ColumnModel(*this)) {}

SummaryModel::~SummaryModel() = default;

scada::aui::HeaderModel& SummaryModel::row_model() {
  return *row_model_;
}

scada::aui::HeaderModel& SummaryModel::column_model() {
  return *column_model_;
}

int SummaryModel::AddColumn(std::string formula) {
  // The cap is on items; a comparison column comes with its item.
  const size_t item_count = static_cast<size_t>(std::ranges::count_if(
      columns_, [](const auto& column) { return !column->comparison(); }));
  if (item_count >= kMaxColumnCount) {
    // Every column holds a history subscription per row, so the cap stays;
    // what changed is that hitting it is said (backlog 962).
    ++dropped_column_count_;
    LOG_WARNING(logger_) << "Column limit reached; the item is not added"
                         << LOG_TAG("Formula", formula)
                         << LOG_TAG("MaxColumnCount", kMaxColumnCount);
    if (notice_changed_handler)
      notice_changed_handler();
    return -1;
  }

  int index = static_cast<int>(columns_.size());
  if (comparison_ != PeriodComparison::kNone) {
    columns_.push_back(std::make_unique<Column>(*this, index + 1, formula,
                                                /*comparison=*/true));
  }
  // Inserted before its comparison column, which was made first only so the
  // formula could still be copied.
  auto& column = *columns_.insert(
      columns_.begin() + index,
      std::make_unique<Column>(*this, index, std::move(formula),
                               /*comparison=*/false));
  column_model_->NotifyModelChanged();

  auto node_id = column->GetNodeId();
  if (!node_id.is_null())
    NotifyContainedItemChanged(node_id, true);

  return index;
}

void SummaryModel::DeleteColumn(int index) {
  // A comparison column goes with its item, and the item with its comparison
  // column: neither means anything alone.
  if (columns_[index]->comparison())
    --index;
  auto node_id = columns_[index]->GetNodeId();

  const bool paired = index + 1 < static_cast<int>(columns_.size()) &&
                      columns_[index + 1]->comparison();
  columns_.erase(columns_.begin() + index,
                 columns_.begin() + index + (paired ? 2 : 1));
  ReindexColumns();
  column_model_->NotifyModelChanged();

  if (!node_id.is_null() && FindColumn(node_id) == -1)
    NotifyContainedItemChanged(node_id, false);
}

int SummaryModel::FindColumn(const scada::NodeId& node_id,
                             int starting_index) const {
  for (int i = starting_index; i < static_cast<int>(columns_.size()); ++i) {
    if (!columns_[i]->comparison() && columns_[i]->GetNodeId() == node_id)
      return i;
  }
  return -1;
}

void SummaryModel::Load(const WindowDefinition& definition) {
  const auto time_range = RestoreTimeRange(definition)
                              .value_or(scada::RelativeTimeRange::Type::Day);
  const auto interval = definition.Get<scada::Duration>("Interval")
                            .value_or(std::chrono::hours(1));
  const scada::NodeId aggregate_type =
      definition.Get<scada::NodeId>("AggregateType")
          .value_or(scada::id::AggregateFunction_End);
  consumption_ = definition.FindItem("Consumption") != nullptr &&
                 aggregate_type == scada::id::AggregateFunction_End;
  comparison_ = PeriodComparison::kNone;
  if (const WindowItem* comparison = definition.FindItem("Comparison"))
    comparison_ = ParsePeriodComparison(comparison->GetString("period"));

  SetParams(time_range,
            scada::AggregateFilter{scada::GetLocalAggregateStartTime(),
                                   interval, aggregate_type});

  // The cap applies per column, not per stored item: it used to count every
  // item of the definition, and drop the columns past it without a word.
  for (const WindowItem& item : definition.items) {
    if (item.name_is("Item")) {
      auto path = item.GetString("path");
      int width = item.GetInt("width", 100);
      int index = AddColumn(std::string{path});
      if (index != -1)
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
    if (column.comparison())
      continue;
    WindowItem& item = definition.AddItem("Item");
    item.SetString("path", column.timed_data().formula());
    item.SetInt("width", column.width());
  }

  SaveTimeRange(definition, GetTimeRange());
  definition.Set("Interval", interval());
  definition.Set("AggregateType", aggregate_type());
  if (consumption_)
    definition.AddItem("Consumption");
  if (comparison_ != PeriodComparison::kNone) {
    definition.AddItem("Comparison")
        .SetString("period", PeriodComparisonName(comparison_));
  }
}

void SummaryModel::GetCell(scada::aui::GridCell& cell) {
  // Blank rather than a number the Server computed some other way; the
  // notice says why (backlog 1015). Blank too while the Server has not yet
  // said which it is, or a print or export taken in that window would carry a
  // 2.5.7 Server's plain sum as the integral (backlog 1029).
  if (IsAggregateWithheld()) {
    cell.text.clear();
    return;
  }

  const Column& column = *columns_[cell.column];

  const scada::DataValue& data_value = column.GetDataValue(cell.row);

  if (IsCustomUnits(aggregate_filter_.aggregate_type)) {
    cell.text = ToString16(data_value.value);
  } else {
    cell.text = column.timed_data().GetValueString(data_value.value,
                                                   data_value.qualifier);
  }

  // A period whose data has not arrived is greyed through the palette's
  // disabled text, so it follows the theme; it used to be a literal
  // Rgba{227, 227, 227} fill that stayed a light stripe on dark (task 367).
  if (!column.IsReady(cell.row)) {
    cell.color_role = scada::aui::ColorRole::Disabled;
  }
}

scada::Time SummaryModel::GetRowTime(int row) const {
  scada::base::Check(row >= 0 && row < static_cast<int>(row_count_));
  scada::base::Check(!scada::IsNull(start_time_));
  scada::base::Check(aggregate_filter_.interval != scada::Duration::zero());
  return start_time_ + aggregate_filter_.interval * row;
}

int SummaryModel::GetRowForTime(scada::Time time) const {
  scada::base::Check(!scada::IsNull(start_time_));
  scada::base::Check(!scada::IsNull(end_time_));
  scada::base::Check(start_time_ <= end_time_);
  scada::base::Check(aggregate_filter_.interval != scada::Duration::zero());

  // |end_time_| defines start of the last interval.
  if (time < start_time_ || time >= end_time_)
    return -1;
  if (row_count_ == 0)
    return -1;

  scada::Duration delta = time - start_time_;
  int row = static_cast<int>(delta / aggregate_filter_.interval);
  scada::base::Check(row >= 0 && row < static_cast<int>(row_count_));
  return row;
}

void SummaryModel::OnCellChanged(int column, int row) {
  NotifyRangeChanged(scada::aui::GridRange::Cell(row, column));
}

void SummaryModel::OnColumnChanged(int column) {
  NotifyRangeChanged(scada::aui::GridRange::Column(column));
  // A column reloading is the sign the session is serving again, so a version
  // probe that failed for another reason is asked again here (backlog 1035).
  ProbeServerVersionIfNeeded();
}

void SummaryModel::OnColumnTitleChanged(int column) {
  column_model_->NotifyModelChanged();
}

void SummaryModel::AddContainedItem(const scada::NodeId& node_id,
                                    unsigned flags) {
  // TOOD: Capture by weak pointer.
  CoSpawn(executor_,
          [this, executor = executor_,
           node = node_service_.GetNode(node_id)]() -> Awaitable<void> {
            return AddContainedItemAsync(*this, executor, node);
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

scada::RelativeTimeRange SummaryModel::GetTimeRange() const {
  return time_range_;
}

void SummaryModel::SetTimeRange(const scada::RelativeTimeRange& time_range) {
  SetParams(time_range, aggregate_filter_);
}

void SummaryModel::SetAggregateType(scada::NodeId aggregate_type) {
  consumption_ = false;
  auto new_filter = aggregate_filter_;
  new_filter.aggregate_type = std::move(aggregate_type);
  SetParams(time_range_, std::move(new_filter));
}

void SummaryModel::SetConsumption() {
  consumption_ = true;
  auto new_filter = aggregate_filter_;
  new_filter.aggregate_type = scada::id::AggregateFunction_End;
  SetParams(time_range_, std::move(new_filter));
}

void SummaryModel::SetComparison(PeriodComparison comparison) {
  if (comparison_ == comparison)
    return;
  comparison_ = comparison;
  // Made anew, so they read with the new shift.
  RebuildComparisonColumns();
  column_model_->NotifyModelChanged();
  NotifyModelChanged();
}

scada::Duration SummaryModel::comparison_shift() const {
  // The shown rows' own span, so the earlier rows line up one to one.
  return PeriodComparisonShift(comparison_, end_time_ - start_time_);
}

bool SummaryModel::IsComparisonColumn(int column) const {
  return IsColumnIndex(column) && columns_[column]->comparison();
}

void SummaryModel::ReindexColumns() {
  for (int i = 0; i < static_cast<int>(columns_.size()); ++i)
    columns_[i]->set_index(i);
}

void SummaryModel::RebuildComparisonColumns() {
  std::erase_if(columns_,
                [](const auto& column) { return column->comparison(); });
  if (comparison_ != PeriodComparison::kNone) {
    std::vector<std::unique_ptr<Column>> columns;
    columns.reserve(columns_.size() * 2);
    for (auto& column : columns_) {
      std::string formula = column->formula();
      const int width = column->width();
      columns.push_back(std::move(column));
      auto& twin = columns.emplace_back(
          std::make_unique<Column>(*this, static_cast<int>(columns.size()),
                                   std::move(formula), /*comparison=*/true));
      twin->set_width(width);
    }
    columns_ = std::move(columns);
  }
  ReindexColumns();
}

void SummaryModel::SetInterval(scada::Duration interval) {
  auto new_filter = aggregate_filter_;
  new_filter.interval = interval;
  SetParams(time_range_, std::move(new_filter));
}

void SummaryModel::SetParams(const scada::RelativeTimeRange& time_range,
                             scada::AggregateFilter aggregate_filter) {
  scada::base::Check(!aggregate_filter.is_null());

  auto params = CalculateSummaryModelParams(
      time_range, aggregate_filter.interval, /*now=*/scada::Now());

  time_range_ = time_range;
  aggregate_filter_ = std::move(aggregate_filter);
  start_time_ = params.start_time;
  end_time_ = params.end_time;
  row_count_ = params.row_count;
  truncated_ = params.truncated;

  LOG_INFO(logger_) << "Calculated params"
                    << LOG_TAG("StartTime", ToString(start_time_))
                    << LOG_TAG("EndTime", ToString(end_time_))
                    << LOG_TAG("RowCount", row_count_)
                    << LOG_TAG("scada::RelativeTimeRange",
                               ToString(time_range_))
                    << LOG_TAG("AggregateFilter", ToString(aggregate_filter_));

  for (size_t i = 0; i < columns_.size(); ++i)
    columns_[i]->UpdateTimes();

  NotifyModelChanged();

  if (notice_changed_handler)
    notice_changed_handler();

  ProbeServerVersionIfNeeded();
}

// static
bool SummaryModel::RequiresServer26(const scada::NodeId& aggregation_id) {
  return aggregation_id == scada::id::AggregateFunction_TimeAverage2 ||
         aggregation_id == scada::id::AggregateFunction_Total ||
         aggregation_id == scada::id::AggregateFunction_Total2;
}

// static
bool SummaryModel::UnreliableBeforeServer26(
    const scada::NodeId& aggregation_id) {
  return aggregation_id == scada::id::AggregateFunction_Average ||
         aggregation_id == scada::id::AggregateFunction_Minimum ||
         aggregation_id == scada::id::AggregateFunction_Maximum ||
         aggregation_id == scada::id::AggregateFunction_Count;
}

bool SummaryModel::IsAggregateUnreliable() const {
  return server_before_2_6_.value_or(false) &&
         UnreliableBeforeServer26(aggregate_filter_.aggregate_type);
}

bool SummaryModel::IsAggregateUnsupported() const {
  return server_before_2_6_.value_or(false) &&
         RequiresServer26(aggregate_filter_.aggregate_type);
}

bool SummaryModel::IsAggregateWithheld() const {
  return RequiresServer26(aggregate_filter_.aggregate_type) &&
         server_before_2_6_.value_or(true);
}

void SummaryModel::ProbeServerVersionIfNeeded() {
  if (server_probe_started_ || server_before_2_6_.has_value() ||
      (!RequiresServer26(aggregate_filter_.aggregate_type) &&
       !UnreliableBeforeServer26(aggregate_filter_.aggregate_type))) {
    return;
  }
  server_probe_started_ = true;

  // The Time Integral gives no error to test on a 2.5.7 Server -- it answers a
  // plain sum -- so the Server is identified by a node 2.6 added instead: the
  // per-item Control object's type, which a 2.5.7 Server refuses as an unknown
  // node like every other 2.6-only node (see services/server_compatibility.h).
  // Answered once per summary, and asked only when an aggregate depends on
  // the answer.
  CoSpawn(executor_, cancelation_,
          [this, cancelation = cancelation_.ref(),
           probe = node_service_.GetNode(
               scada::data_items::id::DataItemControlType)]() mutable
              -> Awaitable<void> {
            const scada::Status status = co_await FetchNodeStatus(probe);
            if (!cancelation.canceled())
              OnServerVersionProbeAnswered(status);
          });
}

void SummaryModel::OnServerVersionProbeAnswered(const scada::Status& status) {
  // Only "unknown node" says 2.5.7, and only a found node says 2.6. Any other
  // failure -- a timeout, the session dropping mid-fetch -- says nothing
  // about the Server, so nothing is decided: the cells stay withheld and the
  // next column change asks again, as the event-severity probe does. Reading
  // such a failure as "2.6" showed a 2.5.7 Server's plain sum as a Time
  // Integral, for good (backlog 1035).
  if (!status && !IsMissingOnServer(status)) {
    LOG_WARNING(logger_) << "Server version probe failed; will ask again"
                         << LOG_TAG("Status", ToString(status));
    server_probe_started_ = false;
    return;
  }
  OnServerVersionProbed(IsMissingOnServer(status));
}

void SummaryModel::OnServerVersionProbed(bool before_2_6) {
  server_before_2_6_ = before_2_6;
  // Either answer changes the cells: a 2.6 Server's values appear, having
  // been withheld while the check was pending (see IsAggregateWithheld).
  NotifyModelChanged();
  if (!before_2_6)
    return;
  if (notice_changed_handler)
    notice_changed_handler();
}

scada::DataValue SummaryModel::GetDataValue(int row, int column) const {
  if (IsAggregateWithheld())
    return {};
  const auto& col = *columns_[column];
  return col.GetDataValue(row);
}

const TimedDataSpec& SummaryModel::timed_data(int column) const {
  return columns_[column]->timed_data();
}

// static
bool SummaryModel::IsCustomUnits(const scada::NodeId& aggregation_id) {
  // Count is a plain number; the time integral is in the item's units times
  // seconds (OPC UA Part 13 §5.4.3.9 Total2,
  // https://reference.opcfoundation.org/Core/Part13/v105/docs/5.4.3.9), so the
  // item's own units would misstate it.
  return aggregation_id == scada::id::AggregateFunction_Count ||
         aggregation_id == scada::id::AggregateFunction_Total;
}

// static
std::u16string SummaryModel::TruncationNotice(scada::Time cut) {
  return u16format(
      Translate("The summary is limited to {} rows and ends at {}."),
      kMaxRowCount,
      UtfConvert<char16_t>(
          FormatTime(cut, TIME_FORMAT_DATE | TIME_FORMAT_TIME)));
}

std::u16string SummaryModel::ColumnLimitNotice(size_t dropped) {
  return u16format(
      Translate("The summary is limited to {} columns; {} items were not "
                "added."),
      kMaxColumnCount, dropped);
}

namespace {

// The method's menu title, for the aggregates UnreliableBeforeServer26 names.
const char* UnreliableAggregateTitle(const scada::NodeId& aggregation_id) {
  if (aggregation_id == scada::id::AggregateFunction_Average)
    return "Average";
  if (aggregation_id == scada::id::AggregateFunction_Minimum)
    return "Minimum";
  if (aggregation_id == scada::id::AggregateFunction_Maximum)
    return "Maximum";
  return "Count";
}

// What a Server before 2.6 gets wrong for `aggregation_id`: only Maximum also
// misreads an all-negative interval.
std::u16string UnreliableAggregateNotice(const scada::NodeId& aggregation_id) {
  return u16format(
      L"{}: {}", Translate(UnreliableAggregateTitle(aggregation_id)),
      aggregation_id == scada::id::AggregateFunction_Maximum
          ? Translate("On a Server before version 2.6 the result may be "
                      "wrong: a maximum below zero is shown as about 0, and "
                      "Bad-quality values are counted too.")
          : Translate("On a Server before version 2.6 the result may be "
                      "wrong: Bad-quality values are counted too."));
}

}  // namespace

std::u16string SummaryModel::GetNotice() const {
  std::u16string notice;
  if (auto cut = truncated_at())
    notice = TruncationNotice(*cut);
  if (dropped_column_count_ != 0) {
    if (!notice.empty())
      notice += u" ";
    notice += ColumnLimitNotice(dropped_column_count_);
  }
  if (IsAggregateUnsupported()) {
    if (!notice.empty())
      notice += u" ";
    // Names the method, since the others work on the same Server.
    notice += u16format(L"{}: {}",
                        aggregate_filter_.aggregate_type ==
                                scada::id::AggregateFunction_TimeAverage2
                            ? Translate("Time Average")
                            : Translate("Time Integral"),
                        ServerTooOldText());
  }
  if (IsAggregateUnreliable()) {
    if (!notice.empty())
      notice += u" ";
    notice += UnreliableAggregateNotice(aggregate_filter_.aggregate_type);
  }
  return notice;
}

std::u16string SummaryModel::GetExportDescription() {
  // The same titles the Summary's own menus show, so a printout names the
  // method the operator chose (backlog 875).
  constexpr std::pair<scada::NumericId, const char*> kAggregateTitles[] = {
      {scada::id::AggregateFunction_Count, "Count"},
      {scada::id::AggregateFunction_Start, "First"},
      {scada::id::AggregateFunction_End, "Last"},
      {scada::id::AggregateFunction_Minimum, "Minimum"},
      {scada::id::AggregateFunction_Maximum, "Maximum"},
      {scada::id::AggregateFunction_Total, "Time Integral"},
      {scada::id::AggregateFunction_Average, "Average"},
      {scada::id::AggregateFunction_TimeAverage2, "Time Average"},
  };
  constexpr std::pair<scada::Duration, const char*> kIntervalTitles[] = {
      {std::chrono::minutes(1), "1-Minute"},
      {std::chrono::minutes(5), "5 min"},
      {std::chrono::minutes(15), "15 min"},
      {std::chrono::minutes(30), "30 min"},
      {std::chrono::hours(1), "1-Hour"},
      {std::chrono::hours(12), "12 hours"},
      {std::chrono::days(1), "1-Day"},
  };

  std::u16string description;
  for (const auto& [id, title] : kAggregateTitles) {
    if (aggregate_filter_.aggregate_type == scada::NodeId{id})
      description = Translate(title);
  }
  // Read through Last, but it is not Last that the cells show.
  if (consumption_)
    description = Translate("Consumption");
  for (const auto& [interval, title] : kIntervalTitles) {
    if (aggregate_filter_.interval == interval) {
      if (!description.empty())
        description += u", ";
      description += Translate(title);
    }
  }
  if (auto cut = truncated_at()) {
    if (!description.empty())
      description += u". ";
    description += TruncationNotice(*cut);
  }
  return description;
}

ExportModel::ExportData SummaryModel::GetExportData() {
  return GridExportData{
      // Translated, and saying which end of the interval the row is: the
      // literal English u"Time" reached a Russian spreadsheet as-is (backlog
      // 912), and a print showed no title at all (backlog 875).
      scada::aui::TableColumn{-1, Translate("Time (interval start)"), 100,
                              scada::aui::TableColumn::LEFT,
                              scada::aui::TableColumn::DataType::General},
      *this, row_model(), column_model()};
}
