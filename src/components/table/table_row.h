#pragma once

#include "common/timed_data/timed_data_spec.h"

class TableModel;

class TableRow : protected rt::TimedDataDelegate {
 public:
  TableRow(TableModel& model, int index);
  ~TableRow();
  
  int index() const { return index_; }    
  void set_index(int index) { index_ = index; }

  const rt::TimedDataSpec& timed_data() const { return timed_data_; }
  bool is_blinking() const { return is_blinking_; }
  
  std::string GetFormula() const;
  base::string16 GetTitle() const;
  
  void SetFormula(const std::string& formula);
  
  void NotifyUpdate();

 protected:
  // rt::TimedDataDelegate
  virtual void OnPropertyChanged(rt::TimedDataSpec& spec,
                                 const rt::PropertySet& properties) override;
  virtual void OnEventsChanged(rt::TimedDataSpec& spec,
                               const events::EventSet& events) override;
  virtual void OnTimedDataNodeModified(rt::TimedDataSpec& spec,
                                       const scada::PropertyIds& property_ids) override;
  virtual void OnTimedDataDeleted(rt::TimedDataSpec& spec) override;

 private:
  void SetBlinking(bool blinking);

  TableModel& model_;
  int index_;
  
  std::string formula_;
  rt::TimedDataSpec timed_data_;
  bool is_blinking_;

  DISALLOW_COPY_AND_ASSIGN(TableRow);
};
