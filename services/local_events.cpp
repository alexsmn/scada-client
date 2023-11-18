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

  const scada::Event& e = *events_.back();
  for (auto& o : observers_)
    o.OnLocalEvent(e);
}

void LocalEvents::AcknowledgeEvent(scada::EventId event_id) {
  auto i = FindEvent(event_id);
  assert(i != events_.end());

  scada::Event& event = **i;
  event.acked = true;
  events_.erase(i);

  for (auto& o : observers_)
    o.OnLocalEvent(event);

  delete &event;
}

void LocalEvents::AcknowledgeAll() {
  Events events;
  events_.swap(events);

  for (scada::Event* event : events) {
    event->acked = true;
    for (auto& o : observers_)
      o.OnLocalEvent(*event);
    delete event;
  }
}

LocalEvents::Events::iterator LocalEvents::FindEvent(scada::EventId event_id) {
  for (auto i = events_.begin(); i != events_.end(); ++i) {
    if ((*i)->event_id == event_id)
      return i;
  }
  return events_.end();
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
