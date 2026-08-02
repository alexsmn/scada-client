#pragma once

#include "aui/models/table_model.h"
#include "base/lifetime.h"
#include "controller/time_model.h"
#include "timed_data/timed_data_spec.h"

namespace scada::base {
class Clock;
}

class TimedDataService;
class WindowDefinition;

struct TimedDataModelContext {
  scada::base::Clock& clock_;
  TimedDataService& timed_data_service_;
  bool utc_time_ = false;
};

class TimedDataModel : private TimedDataModelContext,
                       public scada::aui::TableModel,
                       public TimeModel {
 public:
  enum { CID_TIME, CID_QUALITY, CID_VALUE, CID_COLLECTION_TIME };

  explicit TimedDataModel(TimedDataModelContext&& context);

  void Init(const WindowDefinition& window_def);

  const scada::DataValue& value(int row) const;

  const TimedDataSpec& timed_data() const SCADA_LIFETIME_BOUND {
    return timed_data_;
  }

  // The formula can be updated after initialization.
  void SetFormula(std::string_view formula);

  // aui::TableModel overrides
  virtual int GetRowCount() override;
  virtual void GetCell(scada::aui::TableCell& cell) override;

  // TimeModel
  virtual scada::RelativeTimeRange GetTimeRange() const override;
  virtual void SetTimeRange(const scada::RelativeTimeRange& time_range) override;

 private:
  void UpdateRows(const scada::TimeRange& range);

  TimedDataSpec timed_data_;

  size_t begin_iterator_ = 0;
  int count_ = 0;

  scada::RelativeTimeRange time_range_;

  // Cannot be null.
  scada::Time end_time_ = scada::kMaxTime;
};
