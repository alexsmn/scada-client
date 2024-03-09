#pragma once

#include "scada/event.h"
#include "services/local_events.h"

#include <boost/range/adaptor/indirected.hpp>
#include <boost/signals2/signal.hpp>

class LocalEventModel {
 public:
  explicit LocalEventModel(LocalEvents& local_events)
      : local_events_{local_events} {
    local_event_connection_ = local_events_.event_signal().connect(
        [this](const scada::Event& event) { on_event(event); });
  }

  auto events() const {
    return local_events_.events() | boost::adaptors::indirected;
  }

  void Ack(scada::EventId event_id) {
    local_events_.AcknowledgeEvent(event_id);
  }

  boost::signals2::signal<void(const scada::Event& event)> on_event;

 private:
  LocalEvents& local_events_;

  boost::signals2::scoped_connection local_event_connection_;
};
