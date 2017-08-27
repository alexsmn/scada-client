#pragma once

#include "timed_data/timed_data_spec.h"
#include "ui/base/models/table_model.h"

class TimedDataModel : public ui::TableModel,
                       private rt::TimedDataDelegate {
 public:
  enum {
    CID_TIME,
    CID_QUALITY,
    CID_VALUE,
    CID_COLLECTION_TIME
  };

  explicit TimedDataModel(TimedDataService& timed_data_service);

  rt::TimedVQMap::const_iterator begin() const { return begin_iterator_; }
  rt::TimedVQMap::const_iterator end() const { return end_iterator_; }
  bool empty() const { return begin_iterator_ == end_iterator_; }

  const rt::TimedDataSpec& timed_data() const { return timed_data_; }
  void SetTimedData(rt::TimedDataSpec timed_data);

  void SetFormula(const std::string& formula);

  void Update();

  void Iterate(int index);
  scada::DataValue GetRowTVQ(int row);

  bool IsWorking() const { return !timed_data_.ready(); }

  // ui::TableModel overrides
  virtual int GetRowCount() override;
  virtual void GetCell(ui::TableCell& cell) override;

 private:
  // rt::TimedDataDelegate
  virtual void OnPropertyChanged(rt::TimedDataSpec& spec,
                                 const rt::PropertySet& properties) override;
  virtual void OnTimedDataCorrections(rt::TimedDataSpec& spec, size_t count,
                                      const scada::DataValue* tvqs) override;
  virtual void OnTimedDataReady(rt::TimedDataSpec& spec) override;
  virtual void OnTimedDataNodeModified(rt::TimedDataSpec& spec,
                                       const scada::PropertyIds& property_ids) override;

  TimedDataService& timed_data_service_;
  rt::TimedDataSpec	timed_data_;
  rt::TimedVQMap::const_iterator cached_iterator_;
  int cached_index_;
  rt::TimedVQMap::const_iterator begin_iterator_;
  rt::TimedVQMap::const_iterator end_iterator_;
  int count_;
};

std::string FormatQuality(scada::Qualifier qualifier);