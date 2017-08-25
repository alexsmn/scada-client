#pragma once

#include "core/SkColor.h"
#include "common/timed_data/timed_data_spec.h"
#include "client/components/sheet/sheet_format.h"

class SheetModel;

class SheetCell : private rt::TimedDataDelegate {
 public:
  SheetCell(SheetModel& view, int row, int column);
  ~SheetCell();

  int column() const { return column_; }
  int row() const { return row_; }
  
  const base::string16& text() const { return text_; }
  bool is_blinking() const { return blinking_; }
  const rt::TimedDataSpec& timed_data() const { return timed_data_; }

  const std::string& formula() const { return formula_; }
  bool SetFormula(const std::string& formula);

  rt::TimedDataSpec timed_data_;

  scoped_refptr<SheetFormat> format_;
  
 private:
  void UpdateTextFromFormula();
  
  void NotifyChanged();

  void SetBlinking(bool blinking);

  // rt::TimedDataDelegate
  virtual void OnEventsChanged(rt::TimedDataSpec& spec,
                               const events::EventSet& events);
  virtual void OnPropertyChanged(rt::TimedDataSpec& spec,
                                 const rt::PropertySet& properties);
 
  SheetModel& model_;
  int row_;
  int column_;

  std::string formula_;
  base::string16 text_;
  
  bool blinking_;

  DISALLOW_COPY_AND_ASSIGN(SheetCell);
};
