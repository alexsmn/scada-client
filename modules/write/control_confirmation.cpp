#include "modules/write/control_confirmation.h"

#include "scada/session_service.h"

bool CanSwitchOffControlConfirmation(
    const scada::SessionService& session_service) {
  return session_service.HasAccessRight(scada::AccessRight::kConfigure);
}

bool IsControlConfirmationRequired(bool profile_confirmation,
                                   bool may_switch_off) {
  return profile_confirmation || !may_switch_off;
}

std::string_view ControlConfirmationAdminOnlyText() {
  return "Only an administrator can switch control confirmation off.";
}
