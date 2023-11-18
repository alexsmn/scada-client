#pragma once

#include "base/observer_list.h"
#include "scada/event.h"

#include <string>
#include <vector>

class LocalEvents {
 public:
  // TODO: Use `scada::EventSeverity` instead of `Severity`.
  enum Severity { SEV_INFO, SEV_WARNING, SEV_ERROR };

  class Observer {
   public:
    virtual void OnLocalEvent(const scada::Event& event) = 0;
  };

  LocalEvents();
  ~LocalEvents();

  LocalEvents(const LocalEvents&) = delete;
  LocalEvents& operator=(const LocalEvents&) = delete;

  using Events = std::vector<scada::Event*>;
  const Events& events() const { return events_; }

  void ReportEvent(Severity severity, const scada::LocalizedText& message);

  void AcknowledgeEvent(scada::EventId event_id);
  void AcknowledgeAll();

  base::ObserverList<Observer>& observers() { return observers_; }

 private:
  Events::iterator FindEvent(scada::EventId event_id);

  static scada::EventSeverity SeverityToEvent(Severity severity);

  scada::EventId next_event_id_ = 1;

  Events events_;

  base::ObserverList<Observer> observers_;
};
