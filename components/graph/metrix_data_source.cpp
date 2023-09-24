#include "components/graph/metrix_data_source.h"

#include "common/data_value_util.h"
#include "model/data_items_node_ids.h"
#include "node_service/node_util.h"
#include "timed_data/timed_data_property.h"

#if defined(UI_QT)
#include "graph_qt/model/graph_types.h"
#endif

namespace {

std::pair<scada::DateTime, scada::DateTime> GetTimeRange(
    const TimedDataSpec& spec) {
  const auto& values = spec.values();
  return !values.empty()
             ? std::pair<scada::DateTime,
                         scada::DateTime>{values.front().source_timestamp,
                                          values.back().source_timestamp}
             : std::pair<scada::DateTime, scada::DateTime>{};
}

scada::DateTime GetLatestTimestamp(const TimedDataSpec& spec) {
  const auto& values = spec.values();
  return !values.empty() ? values.back().source_timestamp : scada::DateTime{};
}

}  // namespace

// MetrixPointEnum

class MetrixPointEnum : public views::PointEnumerator {
 public:
  explicit MetrixPointEnum(TimedDataSpec& timed_data)
      : timed_data_(timed_data) {}

  // Returns false if there is no data and iterator is invalid.
  bool Reset(double from,
             double to,
             bool include_left_bound,
             bool include_right_bound);

  // PointEnumerator
  virtual size_t GetCount() const override;
  virtual bool EnumNext(views::GraphPoint& point) override;

 private:
  TimedDataSpec& timed_data_;
  size_t current_position_ = 0;
  size_t last_position_ = 0;
  std::size_t count_ = 0;
  double enum_right_bound_;
  bool enum_include_right_bound_;
  bool enum_current_passed_;

  // Time of last returned value. Supposed for detection if current value is
  // duplicate of last historical value.
  base::Time last_value_time_;
};

bool MetrixPointEnum::Reset(double x_from,
                            double x_to,
                            bool include_left_bound,
                            bool include_right_bound) {
  count_ = 0;
  last_value_time_ = base::Time();
  enum_right_bound_ = x_to;
  enum_include_right_bound_ = include_right_bound;
  enum_current_passed_ = false;

  const auto& values = timed_data_.values();

  current_position_ = LowerBound(values, base::Time::FromDoubleT(x_from));
  if (include_left_bound && current_position_ != 0) {
    --current_position_;
  }

  last_position_ = UpperBound(values, base::Time::FromDoubleT(x_to));
  if (include_right_bound && last_position_ != values.size()) {
    ++last_position_;
  }

  count_ += last_position_ - current_position_;

  if (!timed_data_.current().is_null()) {
    if (values.empty() || values.back().source_timestamp <
                              timed_data_.current().source_timestamp) {
      ++count_;
    }
  }

  return true;
}

size_t MetrixPointEnum::GetCount() const {
  return count_;
}

bool MetrixPointEnum::EnumNext(views::GraphPoint& point) {
  if (count_ == 0)
    return false;

  const auto& values = timed_data_.values();
  if (current_position_ != last_position_) {
    const auto& data_value = values[current_position_];
    // TODO: Duplicate code block.
    point.x = data_value.source_timestamp.ToDoubleT();
    point.y = data_value.value.get_or(0.0);
    point.good = data_value.qualifier.good();
    last_value_time_ = data_value.source_timestamp;
    ++current_position_;

  } else if (!enum_current_passed_) {
    enum_current_passed_ = true;

    const scada::DataValue& current = timed_data_.current();
    if (last_value_time_ >= current.source_timestamp)
      return false;

    // TODO: Duplicate code block.
    point.x = current.source_timestamp.ToDoubleT();
    point.y = current.value.get_or(0.0);
    point.good = current.qualifier.good();

  } else
    return false;

  if (point.x <= enum_right_bound_)
    return true;

  if (enum_include_right_bound_) {
    enum_include_right_bound_ = false;
    return true;
  }

  return false;
}

// MetrixDataSource

MetrixDataSource::MetrixDataSource() {
  timed_data_.ready_handler = [this] { OnHistoryChanged(); };
  timed_data_.correction_handler = [this](size_t count,
                                          const scada::DataValue* tvqs) {
    OnHistoryChanged();
  };
  timed_data_.node_modified_handler = [this] { OnItemChanged(); };
  timed_data_.property_change_handler = [this](const PropertySet& properties) {
    OnPropertyChanged(properties);
  };
  timed_data_.deletion_handler = [this] {
    if (observer_)
      observer_->OnDataSourceDeleted();
  };
}

MetrixDataSource::~MetrixDataSource() = default;

void MetrixDataSource::SetTimedData(const TimedDataSpec& spec) {
  timed_data_ = spec;
  OnItemChanged();
}

void MetrixDataSource::SetRange(const scada::DateTimeRange& range) {
  timed_data_.SetRange(range);
}

bool MetrixDataSource::XToData(double& x, scada::DataValue& val) const {
  if (!connected())
    return false;

  val = timed_data_.GetValueAt(base::Time::FromDoubleT(x));
  return !val.is_null();
}

std::unique_ptr<views::PointEnumerator> MetrixDataSource::EnumPoints(
    double from,
    double to,
    bool include_left_bound,
    bool include_right_bound) {
  auto point_enum = std::make_unique<MetrixPointEnum>(timed_data_);
  bool has_data =
      point_enum->Reset(from, to, include_left_bound, include_right_bound);
  return has_data ? std::move(point_enum) : nullptr;
}

void MetrixDataSource::UpdateRange() {
  if (timed_data_.logical()) {
    range_ = views::GraphRange::Logical();
    return;
  }

  range_ = {};

  if (auto node = timed_data_.node();
      IsInstanceOf(node, data_items::id::AnalogItemType)) {
    range_ = views::GraphRange(
        node[data_items::id::AnalogItemType_EuLo].value().get_or(
            views::kGraphUnknownValue),
        node[data_items::id::AnalogItemType_EuHi].value().get_or(
            views::kGraphUnknownValue));
  }
}

void MetrixDataSource::UpdateLimits() {
  if (auto node = timed_data_.node()) {
    limit_lo_ = node[data_items::id::AnalogItemType_LimitLo].value().get_or(
        views::kGraphUnknownValue);
    limit_hi_ = node[data_items::id::AnalogItemType_LimitHi].value().get_or(
        views::kGraphUnknownValue);
    limit_lolo_ = node[data_items::id::AnalogItemType_LimitLoLo].value().get_or(
        views::kGraphUnknownValue);
    limit_hihi_ = node[data_items::id::AnalogItemType_LimitHiHi].value().get_or(
        views::kGraphUnknownValue);
  } else {
    limit_lo_ = views::kGraphUnknownValue;
    limit_hi_ = views::kGraphUnknownValue;
    limit_lolo_ = views::kGraphUnknownValue;
    limit_hihi_ = views::kGraphUnknownValue;
  }
}

void MetrixDataSource::OnItemChanged() {
  title_ = timed_data_.GetTitle();
  UpdateRange();
  UpdateLimits();

  SetEarliestTimestamp(GetTimeRange(timed_data_).first);
  ScheduleUpdateEarliestTimestamp();

  if (observer_)
    observer_->OnDataSourceItemChanged();

  const auto& tvq = timed_data_.current();
  double value = tvq.value.get_or(views::kGraphUnknownValue);
  SetCurrentValue(value);
}

void MetrixDataSource::OnHistoryChanged() {
  if (observer_)
    observer_->OnDataSourceHistoryChanged();
}

void MetrixDataSource::OnPropertyChanged(const PropertySet& properties) {
  if (properties.is_current_changed()) {
    if (timed_data_.historical())
      OnHistoryChanged();

    const auto& tvq = timed_data_.current();
    double value = tvq.value.get_or(views::kGraphUnknownValue);
    SetCurrentValue(value);
  }

  if (properties.is_item_changed())
    OnItemChanged();

  if (properties.is_title_changed()) {
    title_ = timed_data_.GetTitle();
    if (observer_)
      observer_->OnDataSourceItemChanged();
  }
}

#if defined(UI_QT)
QString MetrixDataSource::GetYAxisLabel(double value) const {
  return QString::fromStdU16String(timed_data_.GetValueString(value, {}));
}
#endif

void MetrixDataSource::SetCurrentValue(double value) {
  if (current_value_ == value)
    return;

  current_value_ = value;

  if (observer_)
    observer_->OnDataSourceCurrentValueChanged();
}

void MetrixDataSource::ScheduleUpdateEarliestTimestamp() {
  update_horizontal_range_cancelation_.Cancel();

  // TODO: Bind executor.
  timed_data_.node()
      .scada_node()
      .read_value_history({.from = scada::DateTime::Min(),
                           .to = scada::DateTime::Max(),
                           .max_count = 1})
      .then(update_horizontal_range_cancelation_.Bind(
          [this](const std::vector<scada::DataValue>& values) {
            SetEarliestTimestamp(values.empty()
                                     ? scada::DateTime{}
                                     : values.front().source_timestamp);
          }));
}

void MetrixDataSource::SetEarliestTimestamp(scada::DateTime timestamp) {
  if (earliest_timestamp_ == timestamp) {
    return;
  }

  earliest_timestamp_ = timestamp;

  if (observer_)
    observer_->OnDataSourceHistoryChanged();
}

views::GraphRange MetrixDataSource::GetHorizontalRange() const {
  auto latest_timestamp = GetTimeRange(timed_data_).second;
  if (earliest_timestamp_.is_null() || latest_timestamp.is_null() ||
      earliest_timestamp_ >= latest_timestamp) {
    return {};
  }

  return {earliest_timestamp_.ToDoubleT(), latest_timestamp.ToDoubleT(),
          views::GraphRange::TIME};
}
