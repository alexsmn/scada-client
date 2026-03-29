#pragma once

#include "base/cancelation.h"
#include "events/event_observer.h"

#include <boost/signals2/connection.hpp>
#include <functional>

class ActionManager;
class Executor;
class LocalEvents;
class NodeEventProvider;
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
                              private EventObserver {
 public:
  explicit EventDispatcher(EventDispatcherContext&& context);
  ~EventDispatcher();

 private:
  void ShowEventsDelayed(bool added);
  void ShowEvents(bool added);

  // EventObserver
  virtual void OnEvents(std::span<const scada::Event* const> events) override;
  virtual void OnAllEventsAcknowledged() override;

  bool playing_alarm_sound_ = false;

  bool has_events_ = false;
  bool showing_events_ = false;
  bool showing_events_added_ = false;

  boost::signals2::scoped_connection local_event_connection_;

  Cancelation cancelation_;
};
