#pragma once
// The ISA-18.2 state of one event row, and the words for it (backlog 1117).
//
// ANSI/ISA-18.2 and IEC 62682 keep an alarm's process state (active, or
// returned to normal) apart from its acknowledgement, and an alarm leaves the
// alarm list only when it is both acknowledged and returned to normal. The
// events panel, the journal, the severity tiles and the escalation ladder all
// read the state from here, so a row's label and the tile it counts towards
// cannot disagree. The model is OPC UA Part 9's: a Condition notification
// carries ActiveState and AckedState (`scada::Event::active`/`acked`), see
// docs/client/ux/principles.md §3 and docs/server/events-opcua-alignment.md §6.
//
// Colour is never the only signal (principles.md §5): every state has a label.
//
// Qt-free and translation-agnostic apart from the label, which goes through
// Translate() like every other UI string.

#include <string>

namespace scada {
struct Event;
}

namespace events {

// One row's alarm state.
enum class AlarmState {
  // A Condition whose limit band stands and nobody has acknowledged.
  kUnackedActive,
  // A Condition still standing that somebody has acknowledged: no longer
  // demanding attention, still wrong. It stays on the alarm list.
  kAckedActive,
  // A Condition that returned to normal before anybody acknowledged it: the
  // alarm cleared unread, and it stays until it is read.
  kUnackedReturned,
  // A Condition notification that has returned to normal and is acknowledged:
  // a journal record only.
  kReturned,
  // A plain event (no Condition, so no process state) nobody has acknowledged.
  // It is taken as standing until acknowledged, which is what every event was
  // before Conditions existed.
  kUnackedEvent,
  // A plain event that is acknowledged: a journal record only.
  kAckedEvent,
};

// The state of `event` as raised: for a Condition, its ActiveState and its
// acknowledgement; for a plain event, its acknowledgement alone.
AlarmState AlarmStateFor(const scada::Event& event);

// Whether `event` counts towards the active severity tiles: a Condition while
// it stands, a plain event until it is acknowledged.
bool IsAlarmActive(const scada::Event& event);

// The translated label for `state`, e.g. "Active, unacknowledged". A plain
// event's label names its acknowledgement too ("Event, unacknowledged"), so
// the label distinguishes every state.
std::u16string AlarmStateLabel(AlarmState state);

}  // namespace events
