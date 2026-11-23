#pragma once

#include "events/severity_tiles.h"

#include <vector>

class LocalEvents;
class NodeEventProvider;

namespace events {

// The alarms the severity tiles and the escalation ladder both count: every
// current server event — unacknowledged events, and Conditions still active
// although acknowledged (backlog 1117) — plus the client's own unacknowledged
// messages at warning severity or above. An info-level client message ("command
// sent") is a record, not an alarm, and counts towards neither.
//
// One function for both consumers because they used to read different sets —
// the ladder counted every local message, the tiles none — so a run of
// successful commands could sound the flood tone over empty tiles (backlog
// 888).
std::vector<AlarmSummary> CollectAlarms(
    const NodeEventProvider& node_event_provider,
    const LocalEvents& local_events);

}  // namespace events
