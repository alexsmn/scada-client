#pragma once

#include <memory>

#include "core/configuration_types.h"
#include "common/timed_data/timed_data.h"
#include "common/timed_data/timed_data_spec.h"

#if defined(UI_QT)
#include "graph_qt/model/graph_data_source.h"
#elif defined(UI_VIEWS)
#include "ui/base/models/graph_data_source.h"
#endif

class MetrixPointEnum;

class MetrixDataSource : public views::GraphDataSource,
                         private rt::TimedDataDelegate {
 public:
  MetrixDataSource();
  virtual ~MetrixDataSource();

  void SetTimedData(const rt::TimedDataSpec& spec);
  void SetFrom(base::Time time) { timed_data_.SetFrom(time); }

  bool is_ready() const { return timed_data_.ready(); }
  scada::NodeId trid() const { return timed_data_.trid(); }
  bool connected() const { return timed_data_.connected(); }
  const rt::TimedDataSpec& timed_data() const { return timed_data_; }
  std::string GetPath() const { return timed_data_.formula(); }
  const base::string16& title() const { return title_; }

  bool XToData(double& x, scada::VQ& val) const;

  // views::GraphDataSource
  virtual views::PointEnumerator* EnumPoints(double from, double to,
                                              bool include_left_bound,
                                              bool include_right_bound);
#if defined(UI_QT)
  virtual std::wstring GetYAxisLabel(double value) const;
#elif defined(UI_VIEWS)
  virtual base::string16 GetYAxisLabel(double value) const;
#endif

 protected:
  void OnItemChanged();
  void OnHistoryChanged();

  void UpdateRange();
  void UpdateLimits();

  // rt::TimedDataDelegate
  virtual void OnTimedDataCorrections(rt::TimedDataSpec& spec, size_t count,
                                      const scada::DataValue* tvqs) override;
  virtual void OnTimedDataReady(rt::TimedDataSpec& spec) override;
  virtual void OnTimedDataNodeModified(rt::TimedDataSpec& spec,
                                       const scada::PropertyIds& property_ids) override;
  virtual void OnTimedDataDeleted(rt::TimedDataSpec& spec) override;
  virtual void OnPropertyChanged(rt::TimedDataSpec& spec,
                                  const rt::PropertySet& properties) override;

  rt::TimedDataSpec timed_data_;
  base::string16 title_;

  std::unique_ptr<MetrixPointEnum> point_enum_;
};
