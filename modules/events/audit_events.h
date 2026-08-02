#pragma once

#include "scada/node_id.h"

// Which events constitute the audit trail.
//
// "Audit" is not one event type: OPC UA Part 5 §6.4.3 defines AuditEventType
// as the base of a subtree, and a server raises concrete subtypes of it. A
// filter that matched one id would silently omit the rest — and an audit log
// that omits entries is worse than no audit log, because it reads as complete.
//
// The client cannot walk the type hierarchy here: this runs in the synchronous
// filter of the event table, where the type nodes may not be resident. What
// makes an explicit list sound instead of a guess is that the set is bounded
// at the other end — the historian writes only the event types listed in its
// own table map (`scada-tier-historian/modules/history/event_types.h`), and
// drops any event whose exact type id is missing there. So this list cannot be
// less complete than what is actually stored; it only has to keep pace with
// that one file.
//
// **Adding an audit type to the historian means adding it here**, or the
// stored events will not reach this view.
[[nodiscard]] bool IsAuditEventType(const scada::NodeId& event_type_id);
