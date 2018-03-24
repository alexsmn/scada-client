#pragma once

#include "base/observer_list.h"
#include "base/strings/string16.h"
#include "core/event.h"

#include <deque>

#undef ReportEvent

class LocalEvents {
 public:
  enum Severity { SEV_INFO, kSeverityWarning, SEV_ERROR };

  class Observer {
   public:
    virtual void OnLocalEvent(const scada::Event& event) = 0;
  };

  LocalEvents();
  ~LocalEvents();

  typedef std::vector<scada::Event*> Events;
  const Events& events() const { return events_; }

  void ReportEvent(Severity severity, const base::string16& message);

  void AcknowledgeEvent(unsigned ack_id);
  void AcknowledgeAll();

  base::ObserverList<Observer>& observers() { return observers_; }

 private:
  Events::iterator FindAckId(unsigned ack_id);

  static scada::EventSeverity SeverityToEvent(Severity severity);

  unsigned next_ack_id_;

  Events events_;

  base::ObserverList<Observer> observers_;

  DISALLOW_COPY_AND_ASSIGN(LocalEvents);
};
