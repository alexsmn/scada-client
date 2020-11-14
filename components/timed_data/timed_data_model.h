#pragma once

#include "time_model.h"
#include "timed_data/timed_data_spec.h"
#include "ui/base/models/table_model.h"

#include <filesystem>

class TimedDataService;

struct TimedDataModelContext {
  TimedDataService& timed_data_service_;
};

class TimedDataModel : private TimedDataModelContext,
                       public ui::TableModel,
                       public TimeModel {
 public:
  enum { CID_TIME, CID_QUALITY, CID_VALUE, CID_COLLECTION_TIME };

  explicit TimedDataModel(TimedDataModelContext&& context);

  const scada::DataValue& value(int row) const;
  int count() const { return count_; }
  bool empty() const { return count_ == 0; }

  const TimedDataSpec& timed_data() const { return timed_data_; }
  void SetTimedData(TimedDataSpec timed_data);

  void SetFormula(std::string_view formula);

  void Update();

  // ui::TableModel overrides
  virtual int GetRowCount() override;
  virtual void GetCell(ui::TableCell& cell) override;

  // TimeModel
  virtual TimeRange GetTimeRange() const override;
  virtual void SetTimeRange(const TimeRange& time_range) override;

 private:
  TimedDataSpec timed_data_;

  size_t begin_iterator_ = 0;
  int count_ = 0;

  TimeRange time_range_;
  // Can be null.
  base::Time end_time_;
};
