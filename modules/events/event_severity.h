#pragma once

// How an event's raw severity maps onto the alarm bands the UI distinguishes.
//
// Qt-free and shared: the journal, the status strip and the KPI tiles all
// bucket severities here, so a threshold change moves every severity surface at
// once — the same reason colours come from aui/severity_colors.h (backlog 0.2).

#include "aui/severity_colors.h"

#include <string>

namespace events {

// Buckets a raw OPC UA event severity (0-1000) into a coarse alarm level:
// >= critical threshold => kCritical, >= warning threshold => kWarning, else
// kNone.
scada::aui::SeverityLevel SeverityLevelForEvent(unsigned severity);

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
