#pragma once

#include "scada/event.h"

#include <boost/signals2/signal.hpp>
#include <string>
#include <vector>

class LocalEvents {
 public:
  // TODO: Use `scada::EventSeverity` instead of `Severity`.
  enum Severity { SEV_INFO, SEV_WARNING, SEV_ERROR };

  LocalEvents();
  ~LocalEvents();

  LocalEvents(const LocalEvents&) = delete;
  LocalEvents& operator=(const LocalEvents&) = delete;

  using Events = std::vector<scada::Event*>;
  const Events& events() const { return events_; }

  void ReportEvent(Severity severity, const scada::LocalizedText& message);

  void AcknowledgeEvent(scada::EventId event_id);
  void AcknowledgeAll();

  using EventSignal = boost::signals2::signal<void(const scada::Event&)>;

  EventSignal& event_signal() { return event_signal_; }

 private:
  Events::iterator FindEvent(scada::EventId event_id);

  static scada::EventSeverity SeverityToEvent(Severity severity);

  scada::EventId next_event_id_ = 1;

  Events events_;

  boost::signals2::signal<void(const scada::Event&)> event_signal_;
};
