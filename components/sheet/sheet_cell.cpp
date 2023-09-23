#include "components/sheet/sheet_cell.h"

#include "aui/models/grid_range.h"
#include "base/strings/sys_string_conversions.h"
#include "components/sheet/sheet_model.h"

SheetCell::SheetCell(SheetModel& model, int row, int column)
    : model_(model), row_(row), column_(column), blinking_(false) {
  timed_data_.event_change_handler = [this] {
    SetBlinking(timed_data_.alerting());
  };
  timed_data_.property_change_handler = [this](const PropertySet& properties) {
    if (properties.is_current_changed())
      UpdateTextFromFormula();
  };
}

SheetCell::~SheetCell() {
  SetBlinking(false);
}

bool SheetCell::SetFormula(std::u16string formula) {
  timed_data_.Reset();
  SetBlinking(false);

  formula_ = std::move(formula);

  bool is_formula = !formula_.empty() && formula_[0] == '=';
  if (is_formula) {
    auto formula2 = ToString(formula_.substr(1));
    timed_data_.Connect(model_.timed_data_service(), formula2);
    SetBlinking(timed_data_.alerting());
    UpdateTextFromFormula();

  } else {
    text_ = formula_;
    NotifyChanged();
  }

  return true;
}

void SheetCell::UpdateTextFromFormula() {
  text_ = timed_data_.GetCurrentString(ValueFormat{0});
  NotifyChanged();
}

void SheetCell::NotifyChanged() {
  model_.NotifyRangeChanged(aui::GridRange::Cell(row_, column_));
}

void SheetCell::SetBlinking(bool blinking) {
  if (blinking_ == blinking)
    return;

  blinking_ = blinking;

  if (blinking_)
    model_.blinking_cells_.insert(this);
  else
    model_.blinking_cells_.erase(this);

  NotifyChanged();
}
