#include "components/table/table_row.h"

#include "components/table/table_model.h"

TableRow::TableRow(TableModel& model, int index)
    : model_{model}, index_{index} {
  timed_data_.node_modified_handler = [this] { NotifyUpdate(); };
  timed_data_.deletion_handler = [this] { NotifyUpdate(); };
  timed_data_.property_change_handler =
      [this](const rt::PropertySet& properties) { NotifyUpdate(); };
  timed_data_.event_change_handler = [this] {
    SetBlinking(timed_data_.alerting());
  };
}

TableRow::~TableRow() {
  SetBlinking(false);
}

std::string TableRow::GetFormula() const {
  return '=' + formula_;
}

base::string16 TableRow::GetTitle() const {
  return timed_data_.GetTitle();
}

void TableRow::SetFormula(std::string formula) {
  formula_ = std::move(formula);
  if (!formula_.empty() && formula_[0] == '=')
    formula_.erase(formula_.begin());

  timed_data_.Connect(model_.timed_data_service(), formula_);

  const events::EventSet* events = timed_data_.GetEvents();
  SetBlinking(events && !events->empty());

  NotifyUpdate();
}

void TableRow::SetBlinking(bool blinking) {
  if (is_blinking_ == blinking)
    return;

  is_blinking_ = blinking;

  if (is_blinking_)
    model_.blinking_rows_.insert(this);
  else
    model_.blinking_rows_.erase(this);

  NotifyUpdate();
}

void TableRow::NotifyUpdate() {
  model_.NotifyItemsChanged(index_, 1);
}
