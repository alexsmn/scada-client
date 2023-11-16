#pragma once

#include "scada/event.h"
#include "services/local_events.h"

#include <boost/range/adaptor/indirected.hpp>
#include <boost/signals2/signal.hpp>

class LocalEventModel : private LocalEvents::Observer {
 public:
  explicit LocalEventModel(LocalEvents& local_events)
      : local_events_{local_events} {
    local_events_.observers().AddObserver(this);
  }

  ~LocalEventModel() { local_events_.observers().RemoveObserver(this); }

  auto events() const {
    return local_events_.events() | boost::adaptors::indirected;
  }

  void Ack(scada::EventId acknowledge_id) {
    local_events_.AcknowledgeEvent(acknowledge_id);
  }

  boost::signals2::signal<void(const scada::Event& event)> on_event;

 private:
  // LocalEvents::Observer
  virtual void OnLocalEvent(const scada::Event& event) override;

  LocalEvents& local_events_;
};

inline void LocalEventModel::OnLocalEvent(const scada::Event& event) {
  on_event(event);
}
