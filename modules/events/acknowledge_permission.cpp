#include "events/acknowledge_permission.h"

#include "aui/translation.h"
#include "scada/authorization.h"
#include "scada/session_service.h"

bool CanAcknowledgeEvents(const scada::SessionService& session_service) {
  return session_service.HasPermission(scada::Permission::kCall);
}

std::u16string AcknowledgeNotPermittedReason() {
  return Translate("Acknowledging requires the Control or Configure privilege");
}
