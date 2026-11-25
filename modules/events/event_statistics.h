#pragma once

#include "aui/severity_colors.h"
#include "scada/node_id.h"

#include <array>
#include <span>
#include <string>
#include <vector>

namespace events {

// One occurrence the event journal shows, reduced to what its statistics
// count. A flood group contributes one sample per occurrence, so the counts
// are of what happened, not of how the journal chose to fold it.
struct EventStatisticsSample {
  // The event's source; null for the Client's own (local) events.
  scada::NodeId object;
  // The name the journal's Object column shows for it.
  std::u16string object_name;
  unsigned severity = 0;
  // Still waiting to be acknowledged (EventTableModel::IsPending).
  bool pending = false;
};

// Event counts over what the journal shows: per alarm band and per object.
// This is the journal's «Статистика»: how many events the period held, how
// they split by severity, and which objects produced them.
struct EventStatistics {
  struct Tally {
    int count = 0;
    int unacknowledged = 0;

    bool operator==(const Tally&) const = default;
  };

  struct ObjectRow {
    scada::NodeId object;
    std::u16string name;
    Tally tally;
    unsigned max_severity = 0;

    bool operator==(const ObjectRow&) const = default;
  };

  Tally total;
  // Indexed by scada::aui::SeverityLevel: routine, warning, critical — the
  // same banding every other severity surface uses (events/event_severity.h).
  std::array<Tally, 3> by_band;
  // Most events first; equal counts by name, the way the operator reads a
  // list (CompareForDisplay).
  std::vector<ObjectRow> by_object;

  const Tally& band(scada::aui::SeverityLevel level) const {
    return by_band[static_cast<size_t>(level)];
  }

  bool operator==(const EventStatistics&) const = default;
};

EventStatistics ComputeEventStatistics(
    std::span<const EventStatisticsSample> samples);

// The band's row title in the statistics: its alarm-band name, or "Routine"
// for the events below the warning threshold. Translated.
std::u16string EventStatisticsBandTitle(scada::aui::SeverityLevel level);

// The statistics as tab-separated text with a header row per table, so a
// paste into a spreadsheet lands one value per cell. `period` heads the text
// when not empty. Translated headings.
std::u16string FormatEventStatisticsText(const EventStatistics& statistics,
                                         std::u16string_view period);

}  // namespace events
