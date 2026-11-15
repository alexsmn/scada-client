#include "modules/table/table_double_click.h"

#include "scada/authorization.h"
#include "scada/session_service.h"

TableDoubleClickAction ChooseTableDoubleClickAction(
    bool row_blinking,
    const scada::SessionService& session_service) {
  if (row_blinking && session_service.HasPermission(scada::Permission::kCall))
    return TableDoubleClickAction::kAcknowledge;
  return TableDoubleClickAction::kDefaultCommand;
}
