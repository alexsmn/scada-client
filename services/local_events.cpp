#include "services/local_events.h"

#include <cassert>

#include "scada/event.h"

LocalEvents::LocalEvents() = default;

LocalEvents::~LocalEvents() {
  for (scada::Event* event : events_) {
    delete event;
  }
}

void LocalEvents::ReportEvent(Severity severity,
                              const scada::LocalizedText& message) {
  scada::EventId event_id = next_event_id_++;
  while ((event_id != 0) && (FindEvent(event_id) != events_.end()))
    event_id = next_event_id_++;

  scada::Event& event = *new scada::Event;
  event.time = base::Time::Now();
  event.message = message;
  event.severity = SeverityToEvent(severity);
  event.event_id = event_id;
  events_.push_back(&event);

  event_signal_(*events_.back());
}

void LocalEvents::AcknowledgeEvent(scada::EventId event_id) {
  auto i = FindEvent(event_id);
  assert(i != events_.end());

  scada::Event& event = **i;
  event.acked = true;
  events_.erase(i);

  event_signal_(event);

  delete &event;
}

void LocalEvents::AcknowledgeAll() {
  Events events;
  events_.swap(events);

  for (scada::Event* event : events) {
    event->acked = true;
    event_signal_(*event);
    delete event;
  }
}

LocalEvents::Events::iterator LocalEvents::FindEvent(scada::EventId event_id) {
  return std::ranges::find(events_, event_id, [](const scada::Event* event) {
    return event->event_id;
  });
}

// static
scada::EventSeverity LocalEvents::SeverityToEvent(Severity severity) {
  switch (severity) {
    case SEV_ERROR:
      return scada::kSeverityCritical;
    case SEV_WARNING:
      return scada::kSeverityWarning;
    case SEV_INFO:
      return scada::kSeverityNormal;
    default:
      assert(false);
      return scada::kSeverityCritical;
  }
}
