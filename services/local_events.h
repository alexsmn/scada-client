#pragma once

#include "base/observer_list.h"
#include "scada/event.h"

#include <deque>
#include <string>

class LocalEvents {
 public:
  enum Severity { SEV_INFO, SEV_WARNING, SEV_ERROR };

  class Observer {
   public:
    virtual void OnLocalEvent(const scada::Event& event) = 0;
  };

  LocalEvents();
  ~LocalEvents();

  LocalEvents(const LocalEvents&) = delete;
  LocalEvents& operator=(const LocalEvents&) = delete;

  typedef std::vector<scada::Event*> Events;
  const Events& events() const { return events_; }

  void ReportEvent(Severity severity, const scada::LocalizedText& message);

  void AcknowledgeEvent(unsigned ack_id);
  void AcknowledgeAll();

  base::ObserverList<Observer>& observers() { return observers_; }

 private:
  Events::iterator FindAckId(unsigned ack_id);

  static scada::EventSeverity SeverityToEvent(Severity severity);

  unsigned next_ack_id_;

  Events events_;

  base::ObserverList<Observer> observers_;
};
