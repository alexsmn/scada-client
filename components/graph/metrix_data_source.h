#pragma once

#include "base/cancelation.h"
#include "common/node_state.h"
#include "timed_data/timed_data.h"
#include "timed_data/timed_data_spec.h"

#if defined(UI_QT)
#include "graph_qt/model/graph_data_source.h"
#endif

#include <memory>

class MetrixPointEnum;

class MetrixDataSource : public views::GraphDataSource {
 public:
  MetrixDataSource();
  virtual ~MetrixDataSource();

  void SetTimedData(const TimedDataSpec& spec);
  void SetRange(const scada::DateTimeRange& range);

  bool is_ready() const { return timed_data_.ready(); }
  scada::NodeId trid() const { return timed_data_.GetNode().node_id(); }
  bool connected() const { return timed_data_.connected(); }
  const TimedDataSpec& timed_data() const { return timed_data_; }
  std::string GetPath() const { return timed_data_.formula(); }
  const std::u16string& title() const { return title_; }

  bool XToData(double& x, scada::DataValue& val) const;

  void SetCurrentValue(double value);

  // views::GraphDataSource
  virtual double GetCurrentValue() const override { return current_value_; };
  virtual views::PointEnumerator* EnumPoints(double from,
                                             double to,
                                             bool include_left_bound,
                                             bool include_right_bound) override;
#if defined(UI_QT)
  virtual QString GetYAxisLabel(double value) const override;
#endif
  virtual views::GraphRange GetHorizontalRange() const override;
  virtual views::GraphRange GetVerticalRange() const override { return range_; }

 protected:
  void OnItemChanged();
  void OnHistoryChanged();

  void UpdateRange();
  void UpdateLimits();

  void ScheduleUpdateEarliestTimestamp();
  void SetEarliestTimestamp(scada::DateTime timestamp);

  void OnPropertyChanged(const PropertySet& properties);

  views::GraphRange range_;
  double current_value_ = views::kGraphUnknownValue;

  TimedDataSpec timed_data_;
  std::u16string title_;

  scada::DateTime earliest_timestamp_;
  Cancelation update_horizontal_range_cancelation_;

  std::unique_ptr<MetrixPointEnum> point_enum_;
};
