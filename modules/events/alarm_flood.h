#pragma once

// Alarm-flood policy: when many alarms are active at once the operator should
// see a single "flood" escalation state rather than reading a raw count off a
// scrolling list (ISA-18.2 alarm-flood management).
//
// ISA-18.2 defines a flood by *rate* (>10 alarms in 10 minutes); this measures
// the standing backlog instead — more than 10 alarms nobody has acknowledged.
// That is a deliberate choice, not an approximation of the rate: the client has
// no arrival-rate signal (NodeEventProvider exposes the current unacknowledged
// set, not an event-rate history), and the backlog is what tells us the
// operator is buried right now, which is the state the escalation exists to
// surface. A rate window would need arrivals timestamped client-side or a
// server-side rate signal.

namespace events {

// Number of unacknowledged alarms above which the situation reads as a flood.
inline constexpr int kAlarmFloodThreshold = 10;

// Whether `unacknowledged_count` unacknowledged alarms constitute a flood.
inline bool IsAlarmFlood(int unacknowledged_count) {
  return unacknowledged_count > kAlarmFloodThreshold;
}

}  // namespace events
