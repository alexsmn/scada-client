#pragma once

#include "base/any_executor.h"
#include "base/cancelation.h"
#include "base/lifetime.h"
#include "common/node_state.h"
#include "timed_data/timed_data.h"
#include "timed_data/timed_data_spec.h"

#include "aui/graph.h"

#include <memory>

class MetrixDataSource : public GraphDataSource {
 public:
  MetrixDataSource();
  explicit MetrixDataSource(AnyExecutor executor);
  virtual ~MetrixDataSource();

  void SetTimedData(const TimedDataSpec& spec);
  void SetRange(const scada::DateTimeRange& range);

  bool is_ready() const { return timed_data_.ready(); }
  scada::NodeId node_id() const { return timed_data_.node_id(); }
  bool connected() const { return timed_data_.connected(); }
  const TimedDataSpec& timed_data() const SCADA_LIFETIME_BOUND {
    return timed_data_;
  }
  std::string GetPath() const { return timed_data_.formula(); }
  const std::u16string& title() const SCADA_LIFETIME_BOUND { return title_; }

  // Configured analog-limit bands (AnalogItemType_LimitLoLo/Lo/Hi/HiHi), or
  // kGraphUnknownValue when the node does not carry that band. Fed to
  // ComputeLimitMarkers to draw limit markers on the pane.
  double limit_lolo() const { return limit_lolo_; }
  double limit_lo() const { return limit_lo_; }
  double limit_hi() const { return limit_hi_; }
  double limit_hihi() const { return limit_hihi_; }

  bool XToData(double& x, scada::DataValue& val) const;

  void SetCurrentValue(double value);

  // GraphDataSource
  virtual double GetCurrentValue() const override { return current_value_; };
  virtual std::unique_ptr<PointEnumerator> EnumPoints(
      double from,
      double to,
      bool include_left_bound,
      bool include_right_bound) override;
#if defined(UI_QT)
  virtual QString GetYAxisLabel(double value) const override;
#endif
  virtual GraphRange GetHorizontalRange() const override;
  virtual GraphRange GetVerticalRange() const override { return range_; }

 protected:
  void OnItemChanged();
  void OnHistoryChanged();

  void UpdateRange();
  void UpdateLimits();

  void ScheduleUpdateEarliestTimestamp();
  void SetEarliestTimestamp(scada::DateTime timestamp);

  void OnPropertyChanged(const PropertySet& properties);

  GraphRange range_;
  double current_value_ = kGraphUnknownValue;

  TimedDataSpec timed_data_;
  std::u16string title_;

  scada::DateTime earliest_timestamp_;
  Cancelation update_horizontal_range_cancelation_;
  AnyExecutor executor_;
};
