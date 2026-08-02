#pragma once

#include "base/lifetime.h"
#include "modules/sheet/sheet_format.h"
#include "timed_data/timed_data_spec.h"

#include <memory>

class SheetModel;

class SheetCell {
 public:
  SheetCell(SheetModel& view, int row, int column);
  ~SheetCell();

  int column() const { return column_; }
  int row() const { return row_; }

  const std::u16string& text() const SCADA_LIFETIME_BOUND { return text_; }

  // The text to paint. A formula cell formats its value here rather than
  // returning what it cached: the item's display format is fetched
  // asynchronously and nothing notifies when it lands, so a cell whose sheet
  // opened before its node loaded kept showing an unformatted number for the
  // life of the window. A literal cell is its text.
  std::u16string GetDisplayText() const;
  bool is_blinking() const { return blinking_; }
  const TimedDataSpec& timed_data() const SCADA_LIFETIME_BOUND {
    return timed_data_;
  }

  const std::u16string& formula() const SCADA_LIFETIME_BOUND {
    return formula_;
  }
  bool SetFormula(std::u16string formula);

  TimedDataSpec timed_data_;

  std::shared_ptr<SheetFormat> format_;

 private:
  void UpdateTextFromFormula();

  void NotifyChanged();

  void SetBlinking(bool blinking);

  SheetModel& model_;
  int row_;
  int column_;

  std::u16string formula_;
  std::u16string text_;

  bool blinking_;

  SheetCell(const SheetCell&) = delete;
  SheetCell& operator=(const SheetCell&) = delete;
};
