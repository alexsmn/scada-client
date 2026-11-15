#pragma once

namespace scada {
class SessionService;
}

// What double-clicking (or pressing Enter on) a table row does.
enum class TableDoubleClickAction {
  // Acknowledge the row's unacknowledged events, which stops it blinking.
  kAcknowledge,
  // Run the row's default node command (usually opening its graph).
  kDefaultCommand,
};

// Chooses the double-click action for a row. A blinking row is acknowledged
// only when the session may acknowledge; otherwise it behaves like any other
// row, so a user who may only watch still gets the graph rather than an
// acknowledgement the server refuses (backlog 1069).
//
// Acknowledging calls AcknowledgeableConditionType_Acknowledge, which the
// server authorizes as a plain Call (it is not one of the process methods
// reserved to the Operator role), so the session's Call permission is the
// exact client-side mirror: Operator or the Configure right grants it,
// Observer alone does not.
TableDoubleClickAction ChooseTableDoubleClickAction(
    bool row_blinking,
    const scada::SessionService& session_service);
