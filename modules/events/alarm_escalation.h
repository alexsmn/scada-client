#pragma once

// The alarm escalation ladder the top context bar draws beside the severity
// tiles (docs/client/ux/shell.md §2.2).
//
// Two rungs, and they are independent conditions rather than levels of one
// scale, so both can be lit at once — the operator is buried *and* something
// critical is unattended:
//
//   1. Annunciation — one or more unacknowledged CRITICAL alarms. This is the
//      ISA-18.2 annunciator: a critical alarm nobody has taken lights it, and
//      any number of warnings does not.
//   2. Flood — more than `kAlarmFloodThreshold` standing unacknowledged alarms
//      of any severity (see alarm_flood.h for why the threshold measures a
//      backlog rather than ISA-18.2's arrival rate).
//
// A flood outranks a single critical and is drawn dominant. Below a rung's
// threshold that rung is absent, not greyed.
//
// The screens this implements are
// docs/product/ui-mockups/screens/operator-shell.html and shell-chrome.html.
// They fix the architecture — the rungs, their triggers and their precedence —
// and deliberately leave the drawing idiom to each realm. The web client's
// half is `alarmEscalation()` in
// web/apps/app/src/features/alarms/alarm-escalation.ts; the two are separate
// products and neither references the other's tree, so the policy is repeated
// rather than shared. Keep them agreeing: a plant escalating at different
// moments depending on which client is open is the defect this note exists to
// prevent.

#include "events/alarm_flood.h"
#include "events/severity_tiles.h"

namespace events {

// Which rungs of the ladder are lit.
struct AlarmEscalation {
  // Rung 1: at least one unacknowledged critical alarm.
  bool annunciating = false;
  // Rung 2: the standing unacknowledged backlog reads as a flood.
  bool flooding = false;

  // Whether either rung is lit.
  bool escalated() const { return annunciating || flooding; }

  friend bool operator==(const AlarmEscalation&,
                         const AlarmEscalation&) = default;
};

// Reduces the tile counts to the escalation state.
//
// Takes `SeverityTileCounts` rather than the alarm set because the context bar
// already has them, and computing the ladder from a second traversal would let
// the chip and the tile beside it disagree about the same alarms.
//
// A caveat worth knowing, because the field name does not say it:
// `counts.critical` is *active* criticals, where rung 1 is defined on
// *unacknowledged* criticals. Those coincide today — the client's event model
// has no cleared-but-unread state, so `NodeEventProvider` retains only
// unacknowledged events and every alarm the counts see is unacknowledged (see
// the note on `CountSeverityTiles`). When that provider gains a
// condition-active flag, this function is one of the places that has to choose
// again, and it must choose the unacknowledged count.
inline AlarmEscalation EscalationFor(const SeverityTileCounts& counts) {
  return AlarmEscalation{
      .annunciating = counts.critical > 0,
      .flooding = IsAlarmFlood(counts.unacknowledged),
  };
}

}  // namespace events
