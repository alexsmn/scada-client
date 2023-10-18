#pragma once

#include "events/event_observer.h"
#include "events/node_event_provider.h"

#include <boost/signals2/signal.hpp>
#include <ranges>

class CurrentEventModel : private EventObserver {
 public:
  explicit CurrentEventModel(NodeEventProvider& node_event_provider)
      : node_event_provider_{node_event_provider} {
    node_event_provider_.AddObserver(*this);
  }

  ~CurrentEventModel() { node_event_provider_.RemoveObserver(*this); }

  auto events() const {
    return node_event_provider_.unacked_events() | std::views::values;
  }

  bool working() const { return node_event_provider_.IsAcking(); }

  void Ack(scada::EventAcknowledgeId ack_id) {
    node_event_provider_.AcknowledgeEvent(ack_id);
  }

  boost::signals2::signal<void()> on_all_acked;
  boost::signals2::signal<void(std::span<const scada::Event* const> events)>
      on_events;

 private:
  virtual void OnEvents(std::span<const scada::Event* const> events) override {
    on_events(events);
  }

  virtual void OnAllEventsAcknowledged() override { on_all_acked(); }

  NodeEventProvider& node_event_provider_;
};
