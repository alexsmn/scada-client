#pragma once

// KPI severity tiles for the operator overview (UX backlog 2.3): the counts
// behind "how bad is it right now", derived from the active alarm set.
//
// Qt-free so the aggregation is unit-testable without a widget, and so the same
// counts can back both the overview tiles and the activity-bar badge. Colours
// come from the severity single source (aui/severity_colors.h, backlog 0.2) so
// a token change restyles the tiles with everything else.
//
// Counts are of *active* alarms — an operator acts on what is currently wrong,
// so a returned-to-normal alarm leaves the tiles even if it is still unread
// (ISA-18.2 distinguishes alarm state from acknowledgement). Unacknowledged is
// tracked separately rather than as a severity, because it answers a different
// question: what has nobody looked at yet.

#include "aui/severity_colors.h"

#include <optional>
#include <span>
#include <string>
#include <vector>

namespace events {

// One alarm as far as the tiles are concerned.
struct AlarmSummary {
  scada::aui::SeverityLevel severity = scada::aui::SeverityLevel::kNone;
  bool acknowledged = false;
  // False once the condition has cleared; such an alarm no longer counts
  // towards the severity tiles.
  bool active = true;
};

// The tile counts for an alarm set.
struct SeverityTileCounts {
  int critical = 0;
  int warning = 0;
  // Active alarms nobody has acknowledged yet, across severities.
  int unacknowledged = 0;

  friend bool operator==(const SeverityTileCounts&,
                         const SeverityTileCounts&) = default;
};

// Aggregates `alarms` into tile counts. Inactive alarms are ignored; alarms of
// severity kNone count towards neither severity tile (but still towards
// unacknowledged, since an unread event is unread whatever its severity).
SeverityTileCounts CountSeverityTiles(std::span<const AlarmSummary> alarms);

// A tile ready to render: caption, count, and the colour the severity single
// source assigns it (nullopt = no severity colour, render in the neutral
// token).
struct SeverityTile {
  // Already translated by the caller (Translate() yields UTF-16, the client's
  // UI string type), so this header stays translation-agnostic.
  std::u16string caption;
  int count = 0;
  std::optional<scada::aui::Color> color;
};

// Builds the tiles in display order (most severe first), so the eye lands on
// the worst state before the detail — High-Performance HMI ordering.
// `caption_*` are supplied by the caller so this stays translation-agnostic
// (UI strings go through Translate() at the widget, never hard-coded here).
std::vector<SeverityTile> BuildSeverityTiles(
    const SeverityTileCounts& counts,
    std::u16string caption_critical,
    std::u16string caption_warning,
    std::u16string caption_unacknowledged);

}  // namespace events
