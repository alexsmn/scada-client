#pragma once

#include "events/event_observer.h"

#include <boost/signals2/connection.hpp>
#include <functional>
#include <string>

class LocalEvents;
class NodeEventProvider;
class Profile;

class EventStatusProvider final : private EventObserver {
 public:
  using ChangeNotifier = std::function<void()>;

  EventStatusProvider(NodeEventProvider& node_event_provider,
                      LocalEvents& local_events,
                      Profile& profile)
      : node_event_provider_{node_event_provider},
        local_events_{local_events},
        profile_{profile} {}

  ~EventStatusProvider();

  void Init(const ChangeNotifier& change_notifier);

  std::u16string GetEventCountText() const;
  std::u16string GetSeverityText() const;

 private:
  // EventObserver
  void OnEvents(std::span<const scada::Event* const> events) override;

  NodeEventProvider& node_event_provider_;
  LocalEvents& local_events_;
  Profile& profile_;

  ChangeNotifier change_notifier_;

  std::vector<boost::signals2::scoped_connection> connections_;
};
