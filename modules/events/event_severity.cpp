#include "events/event_severity.h"

#include "aui/translation.h"
#include "base/format.h"
#include "scada/event.h"

namespace events {

scada::aui::SeverityLevel SeverityLevelForEvent(unsigned severity) {
  if (severity >= scada::kSeverityCritical)
    return scada::aui::SeverityLevel::kCritical;
  if (severity >= scada::kSeverityWarning)
    return scada::aui::SeverityLevel::kWarning;
  return scada::aui::SeverityLevel::kNone;
}

std::optional<scada::aui::EventBackground> EventBackgroundForSeverity(
    unsigned severity) {
  switch (SeverityLevelForEvent(severity)) {
    case scada::aui::SeverityLevel::kCritical:
      return scada::aui::EventBackground::kCritical;
    case scada::aui::SeverityLevel::kWarning:
      return scada::aui::EventBackground::kWarning;
    case scada::aui::SeverityLevel::kNone:
      return std::nullopt;
  }
  return std::nullopt;
}

std::u16string SeverityLevelLabel(scada::aui::SeverityLevel level) {
  // English literals through Translate(); the Russian lives in the .ts.
  switch (level) {
    case scada::aui::SeverityLevel::kCritical:
      return Translate("Critical");
    case scada::aui::SeverityLevel::kWarning:
      return Translate("Warning");
    case scada::aui::SeverityLevel::kNone:
      return {};
  }
  return {};
}

std::u16string EventSeverityLabel(unsigned severity) {
  return SeverityLevelLabel(SeverityLevelForEvent(severity));
}

std::u16string AlarmSummaryLabel(int unacknowledged, unsigned max_severity) {
  if (unacknowledged <= 0)
    return Translate("No unacknowledged events");

  std::u16string label =
      Translate("Unacknowledged") + u": " + WideFormat(unacknowledged);
  label += u" · " + Translate("highest") + u": ";
  if (const std::u16string band = EventSeverityLabel(max_severity);
      !band.empty()) {
    label += band + u" ";
  }
  label += WideFormat(max_severity);
  return label;
}

}  // namespace events
