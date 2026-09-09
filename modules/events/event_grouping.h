#pragma once

// Alarm-flood grouping (UX backlog 2.5): during a flood the journal should read
// as counts, not as a scroll. Repeats of the same alarm collapse into one row
// carrying an occurrence count, so a chattering source costs one line instead
// of fifty and the alarms that only happened once stay visible among them
// (ISA-18.2 flood management — make the condition obvious rather than scrolling
// it past).
//
// Qt-free so the collapse rule is unit-testable without a table, and so the
// same rule can back any other flood surface.

#include "scada/event.h"

#include <span>
#include <string>
#include <utility>
#include <vector>

namespace events {

// A collapsed run of one alarm repeating.
struct EventGroup {
  // The occurrence the row stands for: the most recent one, since during a
  // flood the operator is looking at what is happening now.
  const scada::Event* representative = nullptr;

  // The other occurrences, newest first. Empty when the alarm happened once —
  // such a row is an ordinary ungrouped row.
  std::vector<const scada::Event*> repeats;

  // Total occurrences collapsed here, including the representative.
  int count() const { return 1 + static_cast<int>(repeats.size()); }
};

// What makes two occurrences "the same alarm": the source and what it said.
// Severity is not part of the key — the same condition reported at a different
// severity is still the same condition, and splitting on it would break the
// collapse exactly when a chattering source escalates. Exposed so a journal
// can key a lookup table on it and answer "which row is this alarm's" without
// comparing against every row (task 721).
using AlarmKey = std::pair<scada::NodeId, std::u16string>;
AlarmKey AlarmKeyOf(const scada::Event& event);

// Whether two occurrences read as the same alarm, i.e. belong in one group.
// This is the grouping key, exposed so a journal that folds an arriving event
// into an existing group uses the same rule as a wholesale regroup.
bool IsSameAlarm(const scada::Event& a, const scada::Event& b);

// Collapses `events` into groups of the same alarm repeating — same source node
// *and* same message, so a source that raises two different alarms still shows
// both, and only literal repetition collapses.
//
// Groups come back ordered by their representative's position in `events`, so a
// caller that passes events in display order keeps that order.
std::vector<EventGroup> GroupRepeatedEvents(
    std::span<const scada::Event* const> events);

// The message a grouped row shows: the alarm's own message with its occurrence
// count, e.g. "КП-02: обрыв связи ×7". Returns `message` unchanged for a single
// occurrence, so an ungrouped row never grows a meaningless "×1".
std::u16string FormatGroupedMessage(const std::u16string& message, int count);

}  // namespace events
