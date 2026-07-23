#pragma once

#include "scada/date_time.h"

#include <string_view>
#include <vector>

namespace scada {
struct Event;
}

namespace events {

// A step in a selected event's lifecycle, shown in the Inspector's History
// section (see the reshell mockup
// client/docs/ui-mockups/screens/event-journal.html).
//
// Only steps the event itself records are offered. The mockup also shows a
// "Notified: operator console, sound" row, which has no backing in the event
// model — nothing records per-event notification — so it is deliberately
// absent rather than invented.
enum class EventTimelineStep {
  // The event happened, at the source.
  kRaised,
  // The server received it. Only reported when it differs from the raise
  // time, where the gap is the delivery delay and worth seeing.
  kReceived,
  // An operator acknowledged it.
  kAcknowledged,
  // Nobody has acknowledged it yet — the open end of the timeline.
  kAwaitingAcknowledgement,
};

struct EventTimelineEntry {
  EventTimelineStep step;
  // Null for kAwaitingAcknowledgement: it has not happened yet.
  scada::Time time = scada::kNullTime;
};

// Builds the lifecycle of `event`, oldest step first. Always starts with
// kRaised and ends with either kAcknowledged or kAwaitingAcknowledgement, so
// the section is never empty and always states where the event stands.
std::vector<EventTimelineEntry> BuildEventTimeline(const scada::Event& event);

// The step's operator-facing label, as an untranslated English key for
// Translate().
std::string_view EventTimelineStepText(EventTimelineStep step);

}  // namespace events
