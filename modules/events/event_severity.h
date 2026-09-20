#pragma once

// How an event's raw severity maps onto the alarm bands the UI distinguishes.
//
// Qt-free and shared: the journal, the status strip and the KPI tiles all
// bucket severities here, so a threshold change moves every severity surface at
// once — the same reason colours come from aui/severity_colors.h (backlog 0.2).

#include "aui/severity_colors.h"

#include <optional>
#include <string>

namespace events {

// Buckets a raw OPC UA event severity (0-1000) into a coarse alarm level:
// >= critical threshold => kCritical, >= warning threshold => kWarning, else
// kNone.
scada::aui::SeverityLevel SeverityLevelForEvent(unsigned severity);

// The row-fill class for a raw event severity, or nullopt for a routine event,
// which gets no fill: a calm surface draws the eye only to alarms
// (docs/client/ux/principles.md §1).
//
// This is the second half of the same banding as `SeverityLevelForEvent` --
// `EventBackground` is the *soft row fill* vocabulary where `SeverityLevel` is
// the *solid cue* one -- and it lives here for the same reason: every surface
// that fills a row by severity resolves it here, so the thresholds cannot
// drift apart. They had: the journal and the device log each carried their own
// `>= kSeverityCritical / >= kSeverityWarning` ladder, and the device log's
// went further and painted two literal `Rgba` fills with no text colour, so
// its rows were unreadable under the dark appearance.
std::optional<scada::aui::EventBackground> EventBackgroundForSeverity(
    unsigned severity);

// The operator-facing name of an alarm band ("Critical" / "Warning"), or empty
// for kNone — a routine event has no alarm band to name. Translated, so callers
// render it directly. Every surface that spells a severity out (status strip,
// KPI tiles, event journal) names it here, so they cannot drift apart.
std::u16string SeverityLevelLabel(scada::aui::SeverityLevel level);

// SeverityLevelLabel() of an event's own severity.
std::u16string EventSeverityLabel(unsigned severity);

// The journal footer's one-line summary of the displayed alarm backlog:
// "Unacknowledged: N · highest: <band> <severity>" (the band name omitted for
// a routine severity), or the calm "No unacknowledged events" at zero.
// Translated; count phrasing avoids plural forms.
std::u16string AlarmSummaryLabel(int unacknowledged, unsigned max_severity);

}  // namespace events
