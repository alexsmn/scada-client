#include "services/local_events.h"

#include <cassert>

#include "core/event.h"

LocalEvents::LocalEvents()
    : next_ack_id_(1) {
}

LocalEvents::~LocalEvents() {
  for (Events::iterator i = events_.begin(); i != events_.end(); ++i)
    delete *i;
}

void LocalEvents::ReportEvent(Severity severity, const base::string16& message) {
  unsigned ack_id = next_ack_id_++;
  while ((ack_id != 0) && (FindAckId(ack_id) != events_.end()))
    ack_id = next_ack_id_++;

  scada::Event& event = *new scada::Event;
  event.time = base::Time::Now();
  event.message = message;
  event.severity = SeverityToEvent(severity);
  event.acknowledge_id = ack_id;
  events_.push_back(&event);

  const scada::Event& e = *events_.back();
  FOR_EACH_OBSERVER(Observer, observers_, OnLocalEvent(e));
}

void LocalEvents::AcknowledgeEvent(unsigned ack_id) {
  Events::iterator i = FindAckId(ack_id);
  assert(i != events_.end());

  scada::Event& event = **i;
  event.acked = true;
  events_.erase(i);

  FOR_EACH_OBSERVER(Observer, observers_, OnLocalEvent(event));

  delete &event;
}

void LocalEvents::AcknowledgeAll() {
  Events events;
  events_.swap(events);

  for (Events::iterator i = events.begin(); i != events.end(); ++i) {
    scada::Event& event = **i;
    event.acked = true;
    FOR_EACH_OBSERVER(Observer, observers_, OnLocalEvent(event));
    delete &event;
  }
}

LocalEvents::Events::iterator LocalEvents::FindAckId(unsigned ack_id) {
  for (Events::iterator i = events_.begin(); i != events_.end(); ++i) {
    if ((*i)->acknowledge_id == ack_id)
      return i;
  }
  return events_.end();
}

// static
scada::EventSeverity LocalEvents::SeverityToEvent(Severity severity) {
  switch (severity) {
    case SEV_ERROR:
      return scada::kSeverityCritical;
    case kSeverityWarning:
      return scada::kSeverityWarning;
    case SEV_INFO:
      return scada::kSeverityNormal;
    default:
      assert(false);
      return scada::kSeverityCritical;
  }
}
