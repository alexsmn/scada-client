#include "events/audit_events.h"

#include "model/namespaces.h"
#include "scada/standard_node_ids.h"

#include <array>
#include <algorithm>

bool IsAuditEventType(const scada::NodeId& event_type_id) {
  // Namespace 0 only: the audit types are standard (Part 5 §6.4.3). A
  // vendor-defined type in another namespace is not part of this trail.
  if (event_type_id.namespace_index() != scada::NamespaceIndexes::NS0 ||
      event_type_id.type() != scada::NodeIdType::Numeric) {
    return false;
  }

  static constexpr std::array kAuditTypes = {
      // The subtree root. A server that raised the base type directly, or a
      // subtype this list does not name, still reads as an audit entry when it
      // arrives tagged as the base.
      scada::id::AuditEventType,
      // Session activation — who logged in, and whether it succeeded.
      scada::id::AuditActivateSessionEventType,
      // Every call of a security-administration method: accounts, credentials
      // and role membership, on success AND on denial.
      scada::id::AuditUpdateMethodEventType,
  };

  return std::ranges::find(kAuditTypes, event_type_id.numeric_id()) !=
         kAuditTypes.end();
}
