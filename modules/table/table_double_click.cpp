#include "modules/table/table_double_click.h"

#include "events/acknowledge_permission.h"

TableDoubleClickAction ChooseTableDoubleClickAction(
    bool row_blinking,
    const scada::SessionService& session_service) {
  if (row_blinking && CanAcknowledgeEvents(session_service))
    return TableDoubleClickAction::kAcknowledge;
  return TableDoubleClickAction::kDefaultCommand;
}
