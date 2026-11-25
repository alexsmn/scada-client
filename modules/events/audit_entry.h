#pragma once

#include <string>
#include <string_view>

namespace scada {
struct Event;
}

namespace events {

// Whether the audited action took effect.
enum class AuditOutcome {
  kApplied,
  kDenied,
  // The event says nothing either way (an audit type the Client does not know
  // how to read).
  kUnknown,
};

// One audit-trail record read for an administrator: what was done, to what,
// and with what result. The actor is the event's user id and is resolved by
// the journal like every other user cell.
struct AuditEntry {
  // The action in the reading session's language («Удаление пользователя»).
  // An action the Client has no name for keeps the Server's own word, so the
  // record is never blank.
  std::u16string action;
  // The account or role acted on, as the Server named it. Empty when the
  // record names none, in which case the journal shows the source node.
  std::u16string subject;
  // True when the call was made on a Role (AddIdentity, RemoveIdentity), so
  // the event's source node is the role the subject joined or left and is
  // worth naming beside it. Every other audited method is called on the
  // UserManagement object or the RoleSet, which says nothing.
  bool names_role = false;
  AuditOutcome outcome = AuditOutcome::kUnknown;
  // The Server's status name for a refused call (`Bad_UserAccessDenied`),
  // empty otherwise.
  std::u16string status;
};

// Splits an audit event into action, subject and outcome.
//
// `message` is the event's message text in the session's language. A method
// audit (AuditUpdateMethodEventType, OPC UA Part 5 §6.4.27 AuditUpdateMethod
// EventType, https://reference.opcfoundation.org/Core/Part5/v105/docs/6.4.27)
// reaches the Client without its MethodId or Status as fields — `scada::Event`
// has no per-type field map — so the Server writes them into the message as
// `<Method>[ <subject>][ — <status>]`, the method name untranslated
// (`scada-server-framework/modules/security/method_audit.cpp`). This reads
// that shape back. Any other audit type (a session activation) carries
// free-text prose: it becomes the action, and its outcome comes from the
// severity the Server raises a failure with.
[[nodiscard]] AuditEntry DescribeAuditEvent(const scada::Event& event,
                                            std::u16string_view message);

// The Result cell: a mark plus a word, so the outcome never rests on colour
// («✓ Выполнено», «✕ Отказано: Bad_UserAccessDenied»). Deliberately not an
// alarm severity — a refused administrative call is a security result, not a
// process condition (client/CLAUDE.md, process semantics).
[[nodiscard]] std::u16string AuditOutcomeLabel(const AuditEntry& entry);

}  // namespace events
