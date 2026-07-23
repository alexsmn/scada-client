#include "modules/events/event_timeline.h"

#include "scada/event.h"

namespace events {

std::vector<EventTimelineEntry> BuildEventTimeline(const scada::Event& event) {
  std::vector<EventTimelineEntry> timeline;
  timeline.push_back({EventTimelineStep::kRaised, event.time});

  // `receive_time` is null until the server has processed the event, and
  // equals the raise time when delivery was immediate — in both cases a row
  // would add noise without adding information.
  if (!scada::base::IsNull(event.receive_time) && event.receive_time != event.time)
    timeline.push_back({EventTimelineStep::kReceived, event.receive_time});

  if (event.acked) {
    timeline.push_back(
        {EventTimelineStep::kAcknowledged, event.acknowledged_time});
  } else {
    timeline.push_back(
        {EventTimelineStep::kAwaitingAcknowledgement, scada::base::kNullTime});
  }

  return timeline;
}

std::string_view EventTimelineStepText(EventTimelineStep step) {
  switch (step) {
    case EventTimelineStep::kRaised:
      return "Raised";
    case EventTimelineStep::kReceived:
      return "Received by the server";
    case EventTimelineStep::kAcknowledged:
      return "Acknowledged";
    case EventTimelineStep::kAwaitingAcknowledgement:
      return "Awaiting acknowledgement";
  }
  return {};
}

}  // namespace events
