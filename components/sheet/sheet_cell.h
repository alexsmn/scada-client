#pragma once

#include "components/sheet/sheet_format.h"
#include "timed_data/timed_data_spec.h"

#include <SkColor.h>
#include <memory>

class SheetModel;

class SheetCell {
 public:
  SheetCell(SheetModel& view, int row, int column);
  ~SheetCell();

  int column() const { return column_; }
  int row() const { return row_; }

  const std::wstring& text() const { return text_; }
  bool is_blinking() const { return blinking_; }
  const TimedDataSpec& timed_data() const { return timed_data_; }

  const std::wstring& formula() const { return formula_; }
  bool SetFormula(std::wstring formula);

  TimedDataSpec timed_data_;

  scoped_refptr<SheetFormat> format_;

 private:
  void UpdateTextFromFormula();

  void NotifyChanged();

  void SetBlinking(bool blinking);

  SheetModel& model_;
  int row_;
  int column_;

  std::wstring formula_;
  std::wstring text_;

  bool blinking_;

  DISALLOW_COPY_AND_ASSIGN(SheetCell);
};
