#pragma once

#include "base/memory/weak_ptr.h"
#include "events/event_observer.h"
#include "services/local_events.h"

#include <functional>

class ActionManager;
class NodeEventProvider;
class Executor;
class Profile;

struct EventDispatcherContext {
  const std::shared_ptr<Executor> executor_;
  NodeEventProvider& node_event_provider_;
  LocalEvents& local_events_;
  Profile& profile_;
  const std::function<void(bool has_events)> events_handler_;
  ActionManager& action_manager_;
};

class EventDispatcher final : private EventDispatcherContext,
                              private EventObserver,
                              private LocalEvents::Observer {
 public:
  explicit EventDispatcher(EventDispatcherContext&& context);
  ~EventDispatcher();

 private:
  void ShowEventsDelayed(bool added);
  void ShowEvents(bool added);

  // EventObserver
  virtual void OnEvents(base::span<const scada::Event* const> events) override;
  virtual void OnAllEventsAcknowledged() override;

  // LocalEvents::Observer
  virtual void OnLocalEvent(const scada::Event& event) override;

  bool playing_alarm_sound_ = false;

  bool has_events_ = false;
  bool showing_events_ = false;
  bool showing_events_added_ = false;

  base::WeakPtrFactory<EventDispatcher> weak_factory_{this};
};
