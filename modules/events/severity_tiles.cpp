#include "events/severity_tiles.h"

#include <utility>

namespace events {

SeverityTileCounts CountSeverityTiles(std::span<const AlarmSummary> alarms) {
  SeverityTileCounts counts;
  for (const AlarmSummary& alarm : alarms) {
    if (!alarm.active) {
      continue;
    }
    switch (alarm.severity) {
      case scada::aui::SeverityLevel::kCritical:
        ++counts.critical;
        break;
      case scada::aui::SeverityLevel::kWarning:
        ++counts.warning;
        break;
      case scada::aui::SeverityLevel::kNone:
        break;
    }
    if (!alarm.acknowledged) {
      ++counts.unacknowledged;
    }
  }
  return counts;
}

std::vector<SeverityTile> BuildSeverityTiles(
    const SeverityTileCounts& counts,
    std::u16string caption_critical,
    std::u16string caption_warning,
    std::u16string caption_unacknowledged) {
  std::vector<SeverityTile> tiles;
  tiles.reserve(3);
  tiles.push_back({.caption = std::move(caption_critical),
                   .count = counts.critical,
                   .color = scada::aui::SeverityColor(
                       scada::aui::SeverityLevel::kCritical)});
  tiles.push_back({.caption = std::move(caption_warning),
                   .count = counts.warning,
                   .color = scada::aui::SeverityColor(
                       scada::aui::SeverityLevel::kWarning)});
  // Unacknowledged is a workflow state, not a severity, so it carries no
  // severity colour — the widget renders it in the neutral token.
  tiles.push_back({.caption = std::move(caption_unacknowledged),
                   .count = counts.unacknowledged,
                   .color = std::nullopt});
  return tiles;
}

}  // namespace events
