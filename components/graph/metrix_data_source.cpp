#include "components/graph/metrix_data_source.h"

#include "base/strings/sys_string_conversions.h"
#include "common/node_util.h"
#include "common/scada_node_ids.h"

#if defined(UI_QT)
#include "graph_qt/model/graph_types.h"
#elif defined(UI_VIEWS)
#include "ui/base/models/graph_data_source.h"
#endif

// MetrixPointEnum

class MetrixPointEnum : public views::PointEnumerator {
 public:
  MetrixPointEnum(rt::TimedDataSpec& timed_data) : timed_data_(timed_data) {}

  // Returns false if there is no data and iterator is invalid.
  bool Reset(double from,
             double to,
             bool include_left_bound,
             bool include_right_bound);

  // PointEnumerator
  virtual size_t GetCount() const;
  virtual bool EnumNext(views::GraphPoint& point);

 private:
  rt::TimedDataSpec& timed_data_;
  DataValues::const_iterator enum_position_;
  DataValues::const_iterator last_;
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

  const auto* values = timed_data_.values();
  if (values) {
    enum_position_ = LowerBound(*values, base::Time::FromDoubleT(x_from));
    if (include_left_bound && enum_position_ != values->begin())
      --enum_position_;

    last_ = UpperBound(*values, base::Time::FromDoubleT(x_to));
    if (include_right_bound && last_ != values->end())
      ++last_;

    count_ += last_ - enum_position_;
  }

  if (!timed_data_.current().is_null()) {
    if (!values || values->empty() ||
        values->back().source_timestamp <
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

  if (enum_position_ != last_) {
    point.x = enum_position_->source_timestamp.ToDoubleT();
    point.y = enum_position_->value.get_or(0.0);
    point.good = enum_position_->qualifier.good();
    last_value_time_ = enum_position_->source_timestamp;
    ++enum_position_;

  } else if (!enum_current_passed_) {
    enum_current_passed_ = true;

    const auto& current = timed_data_.current();
    if (last_value_time_ >= current.source_timestamp)
      return false;

    point.x = current.source_timestamp.ToDoubleT();
    point.y = current.value.get_or(0.0);
    point.good = timed_data_.current().qualifier.good();

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
  timed_data_.property_change_handler =
      [this](const rt::PropertySet& properties) {
        OnPropertyChanged(properties);
      };
  timed_data_.deletion_handler = [this] {
    if (observer_)
      observer_->OnDataSourceDeleted();
  };
}

MetrixDataSource::~MetrixDataSource() {}

void MetrixDataSource::SetTimedData(const rt::TimedDataSpec& spec) {
  timed_data_ = spec;
  OnItemChanged();
}

bool MetrixDataSource::XToData(double& x, scada::DataValue& val) const {
  if (!connected())
    return false;

  val = timed_data_.GetValueAt(base::Time::FromDoubleT(x));
  return !val.is_null();
}

views::PointEnumerator* MetrixDataSource::EnumPoints(double from,
                                                     double to,
                                                     bool include_left_bound,
                                                     bool include_right_bound) {
  if (!point_enum_.get())
    point_enum_.reset(new MetrixPointEnum(timed_data_));
  bool has_data =
      point_enum_->Reset(from, to, include_left_bound, include_right_bound);
  return has_data ? point_enum_.get() : NULL;
}

void MetrixDataSource::UpdateRange() {
  if (timed_data_.logical()) {
    range_ = views::GraphRange::Logical();
    return;
  }

  range_ = views::GraphRange();

  auto node = timed_data_.GetNode();
  if (IsInstanceOf(node, id::AnalogItemType)) {
    range_ = views::GraphRange(
        node[id::AnalogItemType_EuLo].value().get_or(views::kGraphUnknownValue),
        node[id::AnalogItemType_EuHi].value().get_or(
            views::kGraphUnknownValue));
  }
}

void MetrixDataSource::UpdateLimits() {
  if (auto node = timed_data_.GetNode()) {
    limit_lo_ = node[id::AnalogItemType_LimitLo].value().get_or(
        views::kGraphUnknownValue);
    limit_hi_ = node[id::AnalogItemType_LimitHi].value().get_or(
        views::kGraphUnknownValue);
    limit_lolo_ = node[id::AnalogItemType_LimitLoLo].value().get_or(
        views::kGraphUnknownValue);
    limit_hihi_ = node[id::AnalogItemType_LimitHiHi].value().get_or(
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

void MetrixDataSource::OnPropertyChanged(const rt::PropertySet& properties) {
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
  return QString::fromStdWString(timed_data_.GetValueString(value, {}));
}
#elif defined(UI_VIEWS)
base::string16 MetrixDataSource::GetYAxisLabel(double value) const {
  return timed_data_.GetValueString(value, {});
}
#endif
