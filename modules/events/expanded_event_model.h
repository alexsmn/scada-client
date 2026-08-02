#pragma once

#include "aui/models/table_model.h"
#include "events/event_table_model.h"

// A read-only view of the journal with its flood groups expanded back into one
// row per occurrence (UX backlog 2.5).
//
// Grouping is a display decision — it keeps a flood readable — but a record of
// the journal is evidence of what happened, so an export or a printout must
// contain every occurrence rather than "…×7". This adapter sits between the
// journal model and the generic export/print machinery, which walks any
// TableModel by row and knows nothing about groups.
class ExpandedEventModel : public scada::aui::TableModel {
 public:
  explicit ExpandedEventModel(EventTableModel& source) : source_{source} {}

  // aui::TableModel
  int GetRowCount() override { return source_.GetOccurrenceCount(); }
  void GetCell(scada::aui::TableCell& cell) override {
    source_.GetOccurrenceCell(cell);
  }

 private:
  EventTableModel& source_;
};
