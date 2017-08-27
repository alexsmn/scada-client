#include "components/graph/metrix_data_source.h"

#include "base/strings/sys_string_conversions.h"
#include "common/scada_node_ids.h"
#include "common/node_ref_util.h"

#if defined(UI_QT)
#include "graph_qt/model/graph_types.h"
#elif defined(UI_VIEWS)
#include "ui/base/models/graph_types.h"
#endif

// MetrixPointEnum

class MetrixPointEnum : public views::PointEnumerator {
 public:
  MetrixPointEnum(rt::TimedDataSpec& timed_data)
      : timed_data_(timed_data) {}
 
  // Returns false if there is no data and iterator is invalid.
  bool Reset(double from, double to, bool include_left_bound,
             bool include_right_bound);

  // PointEnumerator
  virtual bool EnumNext(views::GraphPoint& point);
  
 private:
  rt::TimedDataSpec& timed_data_;
  rt::TimedVQMap::const_iterator enum_position_;
  double enum_right_bound_;
  bool enum_include_right_bound_;
  bool enum_current_passed_;
  
  // Time of last returned value. Supposed for detection if current value is
  // duplicate of last historical value.
  base::Time last_value_time_;
};

bool MetrixPointEnum::Reset(double x_from, double x_to,
                            bool include_left_bound,
                            bool include_right_bound) {
  if (!timed_data_.connected())
    return false;

  const rt::TimedVQMap* values = timed_data_.values();
  if (!values)
    return false;
 
  enum_right_bound_ = x_to;
  enum_include_right_bound_ = include_right_bound;
  enum_current_passed_ = false;

  enum_position_ = values->lower_bound(base::Time::FromDoubleT(x_from));
  if (include_left_bound && enum_position_ != values->begin())
    --enum_position_;
    
  last_value_time_ = base::Time();
  
  return true;
}

bool MetrixPointEnum::EnumNext(views::GraphPoint& point) {
  const rt::TimedVQMap* values = timed_data_.values();
  assert(values);

  if (enum_position_ != values->end()) {
    point.x = enum_position_->first.ToDoubleT();
    point.y = enum_position_->second.vq.value.get_or(0.0);
    point.good = enum_position_->second.vq.qualifier.good();
    last_value_time_ = enum_position_->first;
    ++enum_position_;
    
  } else if (!enum_current_passed_) {
    enum_current_passed_ = true;
    
    const auto& current = timed_data_.current();
    if (last_value_time_ == current.time)
      return false;
    
    point.x = current.time.ToDoubleT();
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
  timed_data_.set_delegate(this);
}

MetrixDataSource::~MetrixDataSource() {
}

void MetrixDataSource::SetTimedData(const rt::TimedDataSpec& spec) {
  timed_data_ = spec;
  OnItemChanged();
}

bool MetrixDataSource::XToData(double& x, scada::VQ& val) const {
  if (!connected())
    return false;

  const auto& value = timed_data_.GetValueAt(base::Time::FromDoubleT(x));
  if (value.is_null())
    return false;

  x = value.time.ToDoubleT();
  val.value = value.value;
  val.qualifier = value.qualifier;
  return true;
}

views::PointEnumerator* MetrixDataSource::EnumPoints(double from, double to,
                                                     bool include_left_bound,
                                                     bool include_right_bound) {
  if (!point_enum_.get())
    point_enum_.reset(new MetrixPointEnum(timed_data_));
  bool has_data = point_enum_->Reset(from, to,
                                     include_left_bound,
                                     include_right_bound);
  return has_data ? point_enum_.get() : NULL;
}

void MetrixDataSource::UpdateRange() {
  if (timed_data_.logical()) {
    range_ = views::GraphRange::Logical();
    return;
  }

  range_ = views::GraphRange();

  const auto& node = timed_data_.GetNode();
  if (IsInstanceOf(node, id::AnalogItemType)) {
    range_ = views::GraphRange(
        node[id::AnalogItemType_EuLo].value().get_or(views::kGraphUnknownValue),
        node[id::AnalogItemType_EuHi].value().get_or(views::kGraphUnknownValue));
  }
}

void MetrixDataSource::UpdateLimits() {
  const auto& node = timed_data_.GetNode();
  if (IsInstanceOf(node, id::AnalogItemType)) {
    limit_lo_ = node[id::AnalogItemType_LimitLo].value().get_or(views::kGraphUnknownValue);
    limit_hi_ = node[id::AnalogItemType_LimitHi].value().get_or(views::kGraphUnknownValue);
    limit_lolo_ = node[id::AnalogItemType_LimitLoLo].value().get_or(views::kGraphUnknownValue);
    limit_hihi_ = node[id::AnalogItemType_LimitHiHi].value().get_or(views::kGraphUnknownValue);
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

void MetrixDataSource::OnTimedDataReady(rt::TimedDataSpec& spec) {
  assert(&spec == &timed_data_);
  OnHistoryChanged();
}

void MetrixDataSource::OnTimedDataCorrections(
    rt::TimedDataSpec& spec, size_t count, const scada::DataValue* tvqs) {
  assert(&spec == &timed_data_);
  assert(count > 0);
  OnHistoryChanged();
}

void MetrixDataSource::OnTimedDataNodeModified(rt::TimedDataSpec& spec,
                                               const scada::PropertyIds& property_ids) {
  OnItemChanged();
}

void MetrixDataSource::OnTimedDataDeleted(rt::TimedDataSpec& spec) {
  if (observer_)
    observer_->OnDataSourceDeleted();
}

void MetrixDataSource::OnPropertyChanged(rt::TimedDataSpec& spec,
                                         const rt::PropertySet& properties) {
  assert(&spec == &timed_data_);

  if (properties.is_current_changed()) {
    if (spec.historical())
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