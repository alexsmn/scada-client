#pragma once

// Alarm-flood policy: when many alarms are active at once the operator should
// see a single "flood" escalation state rather than reading a raw count off a
// scrolling list (ISA-18.2 alarm-flood management). The threshold mirrors the
// UX spec's ">10 in 10 min" — here applied to the count of currently
// unacknowledged alarms, which is the always-available signal.

// Number of unacknowledged alarms above which the situation reads as a flood.
inline constexpr int kAlarmFloodThreshold = 10;

// Whether `unacknowledged_count` unacknowledged alarms constitute a flood.
inline bool IsAlarmFlood(int unacknowledged_count) {
  return unacknowledged_count > kAlarmFloodThreshold;
}
