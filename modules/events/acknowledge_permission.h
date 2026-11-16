#pragma once

#include <string>

namespace scada {
class SessionService;
}

// Whether the session may acknowledge events.
//
// Acknowledging calls AcknowledgeableConditionType_Acknowledge, which the
// server authorizes as a plain Call through the role defaults — it is not one
// of the process methods reserved to the Operator role — so the session's Call
// permission is the exact client-side mirror: the Operator role or the
// Configure right grants it, Observer alone does not. Every surface that
// acknowledges asks this, so an account that may only watch is never offered
// an acknowledgement the server refuses (backlog 1069, 1070).
bool CanAcknowledgeEvents(const scada::SessionService& session_service);

// The disabled reason an acknowledge command shows when CanAcknowledgeEvents()
// is false.
std::u16string AcknowledgeNotPermittedReason();
