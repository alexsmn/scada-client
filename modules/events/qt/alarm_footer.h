#pragma once

#include "modules/events/event_table_model.h"

#include <functional>

class CommandHandler;
class QWidget;

// Inputs for the journal's alarm-footer strip (Qt): the mockup's "N
// unacknowledged · highest Critical" summary plus the Acknowledge-all action
// (the events surface of docs/product/ui-mockups/screens/operator-shell.html).
//
// The strip re-reads `summary` whenever `model` notifies a change, so the
// count tracks arrivals, acknowledgements and refilters. `acknowledge_all`
// resolves the journal's Acknowledge-All command; the button follows its
// enablement (it acknowledges the *active* alarms — a displayed historical
// event that was never acknowledged stays in the count but cannot be
// acknowledged from the client).
struct AlarmFooterContext {
  scada::aui::TableModel& model;
  std::function<EventTableModel::AlarmSummary()> summary;
  std::function<CommandHandler*()> acknowledge_all;
};

// Builds the footer strip. The caller gates it on the
// active UX theme (the journal only, not the docked panel).
QWidget* MakeAlarmFooter(AlarmFooterContext context);
