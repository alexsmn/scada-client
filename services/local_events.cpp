#include "services/local_events.h"

#include <cassert>

#include "scada/event.h"

LocalEvents::LocalEvents() = default;

LocalEvents::~LocalEvents() {
  for (Events::iterator i = events_.begin(); i != events_.end(); ++i)
    delete *i;
}

void LocalEvents::ReportEvent(Severity severity,
                              const scada::LocalizedText& message) {
  scada::EventAcknowledgeId ack_id = next_ack_id_++;
  while ((ack_id != 0) && (FindAckId(ack_id) != events_.end()))
    ack_id = next_ack_id_++;

  scada::Event& event = *new scada::Event;
  event.time = base::Time::Now();
  event.message = message;
  event.severity = SeverityToEvent(severity);
  event.acknowledge_id = ack_id;
  events_.push_back(&event);

  const scada::Event& e = *events_.back();
  for (auto& o : observers_)
    o.OnLocalEvent(e);
}

void LocalEvents::AcknowledgeEvent(scada::EventAcknowledgeId ack_id) {
  auto i = FindAckId(ack_id);
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

  for (auto i = events.begin(); i != events.end(); ++i) {
    scada::Event& event = **i;
    event.acked = true;
    for (auto& o : observers_)
      o.OnLocalEvent(event);
    delete &event;
  }
}

LocalEvents::Events::iterator LocalEvents::FindAckId(
    scada::EventAcknowledgeId ack_id) {
  for (auto i = events_.begin(); i != events_.end(); ++i) {
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
    case SEV_WARNING:
      return scada::kSeverityWarning;
    case SEV_INFO:
      return scada::kSeverityNormal;
    default:
      assert(false);
      return scada::kSeverityCritical;
  }
}
