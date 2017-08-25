#include "client/components/sheet/sheet_cell.h"

#include "base/strings/sys_string_conversions.h"
#include "client/components/sheet/sheet_model.h"
#include "common/event_manager.h"
#include "ui/base/models/grid_range.h"

SheetCell::SheetCell(SheetModel& model, int row, int column)
    : model_(model),
      row_(row),
      column_(column),
      blinking_(false) {
  timed_data_.set_delegate(this);
}

SheetCell::~SheetCell() {
  SetBlinking(false);
}

void SheetCell::OnEventsChanged(rt::TimedDataSpec& spec,
                                const events::EventSet& events) {
  SetBlinking(!events.empty());
}

void SheetCell::OnPropertyChanged(rt::TimedDataSpec& spec,
                                  const rt::PropertySet& properties) {
  if (properties.is_current_changed())
    UpdateTextFromFormula();
}

bool SheetCell::SetFormula(const std::string& formula) {
  timed_data_.Reset();
  SetBlinking(false);
  
  formula_ = formula;

  bool is_formula = !formula.empty() && formula[0] == '=';
  if (is_formula) {
    std::string formula2 = formula.c_str() + 1;
    
    try {
      timed_data_.Connect(model_.timed_data_service(), formula2);
      
    } catch (const std::exception&) {
      text_ = L"#Œÿ»¡ ¿?";
      NotifyChanged();
      return false;
    }
    
    SetBlinking(timed_data_.alerting());
    UpdateTextFromFormula();
    
  } else {
    text_ = base::SysNativeMBToWide(formula);
    NotifyChanged();
  }

  return true;
}

void SheetCell::UpdateTextFromFormula() {
  text_ = timed_data_.GetCurrentString(0);
  NotifyChanged();
}

void SheetCell::NotifyChanged() {
  model_.NotifyRangeChanged(ui::GridRange::Cell(row_, column_));
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
