#pragma once

#include <memory>

#include "core/configuration_types.h"
#include "timed_data/timed_data.h"
#include "timed_data/timed_data_spec.h"

#if defined(UI_QT)
#include "graph_qt/model/graph_data_source.h"
#elif defined(UI_VIEWS)
#include "ui/base/models/graph_data_source.h"
#include "ui/base/models/graph_types.h"
#endif

class MetrixPointEnum;

class MetrixDataSource : public views::GraphDataSource {
 public:
  MetrixDataSource();
  virtual ~MetrixDataSource();

  void SetTimedData(const rt::TimedDataSpec& spec);
  void SetRange(const scada::DateTimeRange& range);

  bool is_ready() const { return timed_data_.ready(); }
  scada::NodeId trid() const { return timed_data_.GetNode().node_id(); }
  bool connected() const { return timed_data_.connected(); }
  const rt::TimedDataSpec& timed_data() const { return timed_data_; }
  std::string GetPath() const { return timed_data_.formula(); }
  const base::string16& title() const { return title_; }

  bool XToData(double& x, scada::DataValue& val) const;

  // views::GraphDataSource
  virtual views::PointEnumerator* EnumPoints(double from,
                                             double to,
                                             bool include_left_bound,
                                             bool include_right_bound);
#if defined(UI_QT)
  virtual QString GetYAxisLabel(double value) const;
#elif defined(UI_VIEWS)
  virtual base::string16 GetYAxisLabel(double value) const;
#endif

 protected:
  void OnItemChanged();
  void OnHistoryChanged();

  void UpdateRange();
  void UpdateLimits();

  void OnPropertyChanged(const rt::PropertySet& properties);

  rt::TimedDataSpec timed_data_;
  base::string16 title_;

  std::unique_ptr<MetrixPointEnum> point_enum_;
};
