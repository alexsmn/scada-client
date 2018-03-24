#pragma once

#include "time_model.h"
#include "timed_data/timed_data_spec.h"
#include "ui/base/models/table_model.h"

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

  rt::TimedVQMap::const_iterator begin() const { return begin_iterator_; }
  rt::TimedVQMap::const_iterator end() const { return end_iterator_; }
  bool empty() const { return begin_iterator_ == end_iterator_; }

  const rt::TimedDataSpec& timed_data() const { return timed_data_; }
  void SetTimedData(rt::TimedDataSpec timed_data);

  void SetFormula(const std::string& formula);

  void Update();

  void Iterate(int index);
  scada::DataValue GetRowTVQ(int row);

  // ui::TableModel overrides
  virtual int GetRowCount() override;
  virtual void GetCell(ui::TableCell& cell) override;

  // TimeModel
  virtual TimeRange GetTimeRange() const override;
  virtual void SetTimeRange(const TimeRange& time_range) override;

 private:
  rt::TimedDataSpec timed_data_;
  rt::TimedVQMap::const_iterator cached_iterator_;
  int cached_index_ = -1;
  rt::TimedVQMap::const_iterator begin_iterator_;
  rt::TimedVQMap::const_iterator end_iterator_;
  int count_ = 0;
  TimeRange time_range_;
  // Can be null.
  base::Time end_time_;
};

base::string16 FormatQuality(scada::Qualifier qualifier);
