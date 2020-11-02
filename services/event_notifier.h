#pragma once

#include "base/memory/weak_ptr.h"
#include "common/event_observer.h"
#include "services/local_events.h"

#include <functional>

class ActionManager;
class EventFetcher;
class Profile;

struct EventNotifierContext {
  EventFetcher& event_fetcher_;
  LocalEvents& local_events_;
  Profile& profile_;
  std::function<void(bool has_events)> events_handler_;
  ActionManager& action_manager_;
};

class EventNotifier final : private EventNotifierContext,
                            private EventObserver,
                            private LocalEvents::Observer {
 public:
  explicit EventNotifier(EventNotifierContext&& context);
  ~EventNotifier();

 private:
  void ShowEventsDelayed(bool added);
  void ShowEvents(bool added);

  // EventObserver
  virtual void OnEventReported(const scada::Event& event) override;
  virtual void OnEventAcknowledged(const scada::Event& event) override;
  virtual void OnAllEventsAcknowledged() override;

  // LocalEvents::Observer
  virtual void OnLocalEvent(const scada::Event& event) override;

  bool playing_alarm_sound_ = false;

  bool has_events_ = false;
  bool showing_events_ = false;
  bool showing_events_added_ = false;

  base::WeakPtrFactory<EventNotifier> weak_factory_{this};
};
