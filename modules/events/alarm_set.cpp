#include "events/alarm_set.h"

#include "events/event_severity.h"
#include "events/local_events.h"
#include "events/node_event_provider.h"
#include "scada/event.h"

#include <ranges>

namespace events {

std::vector<AlarmSummary> CollectAlarms(
    const NodeEventProvider& node_event_provider,
    const LocalEvents& local_events) {
  std::vector<AlarmSummary> alarms;
  alarms.reserve(node_event_provider.unacked_events().size() +
                 local_events.events().size());

  auto account = [&alarms](const scada::Event& event) {
    alarms.push_back(AlarmSummary{
        .severity = SeverityLevelForEvent(event.severity),
        .acknowledged = event.acked,
    });
  };

  for (const scada::Event& event :
       node_event_provider.unacked_events() | std::views::values) {
    account(event);
  }
  for (const scada::Event* event : local_events.events()) {
    if (!event->acked && event->severity >= scada::kSeverityWarning)
      account(*event);
  }

  return alarms;
}

}  // namespace events
