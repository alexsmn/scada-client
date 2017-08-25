#include "client/components/table/table_row.h"

#include "base/strings/sys_string_conversions.h"
#include "base/format_time.h"
#include "client/components/table/table_model.h"
#include "core/monitored_item_service.h"
#include "common/event_manager.h"

extern int g_time_format;

TableRow::TableRow(TableModel& model, int index)
    : model_(model),
      index_(index),
      is_blinking_(false) {
  timed_data_.set_delegate(this);
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

void TableRow::SetFormula(const std::string& formula) {
  formula_ = formula;
  if (!formula_.empty() && formula_[0] == '=')
    formula_.erase(formula_.begin());
  
  try {
    timed_data_.Connect(model_.timed_data_service(), formula_);
  } catch (const std::exception&) {
  }
  
  const events::EventSet* events = timed_data_.GetEvents();
  SetBlinking(events && !events->empty());

  NotifyUpdate();
}

void TableRow::OnTimedDataNodeModified(rt::TimedDataSpec& spec,
                                       const scada::PropertyIds& property_ids) {
  NotifyUpdate();
}

void TableRow::OnTimedDataDeleted(rt::TimedDataSpec& spec) {
  NotifyUpdate();
}

void TableRow::OnPropertyChanged(rt::TimedDataSpec& spec,
                                     const rt::PropertySet& properties) {
  NotifyUpdate();
}

void TableRow::OnEventsChanged(rt::TimedDataSpec& spec,
                                     const events::EventSet& events) {
  SetBlinking(!events.empty());
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
