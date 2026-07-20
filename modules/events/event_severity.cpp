#include "events/event_severity.h"

#include "aui/translation.h"
#include "scada/event.h"

namespace events {

scada::aui::SeverityLevel SeverityLevelForEvent(unsigned severity) {
  if (severity >= scada::kSeverityCritical)
    return scada::aui::SeverityLevel::kCritical;
  if (severity >= scada::kSeverityWarning)
    return scada::aui::SeverityLevel::kWarning;
  return scada::aui::SeverityLevel::kNone;
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

}  // namespace events
