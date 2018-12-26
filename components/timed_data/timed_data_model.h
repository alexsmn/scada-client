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

  DataValues::const_iterator begin() const { return begin_iterator_; }
  DataValues::const_iterator end() const { return begin_iterator_ + count_; }

  const scada::DataValue& value(int row) const;
  int count() const { return count_; }
  bool empty() const { return count_ == 0; }

  const rt::TimedDataSpec& timed_data() const { return timed_data_; }
  void SetTimedData(rt::TimedDataSpec timed_data);

  void SetFormula(base::StringPiece formula);

  void Update();

  void ExportToCsv(const std::filesystem::path& path);

  // ui::TableModel overrides
  virtual int GetRowCount() override;
  virtual void GetCell(ui::TableCell& cell) override;

  // TimeModel
  virtual TimeRange GetTimeRange() const override;
  virtual void SetTimeRange(const TimeRange& time_range) override;

 private:
  rt::TimedDataSpec timed_data_;

  DataValues::const_iterator begin_iterator_;
  int count_ = 0;

  TimeRange time_range_;
  // Can be null.
  base::Time end_time_;
};
