#pragma once

#include "base/memory/weak_ptr.h"
#include "common/event_observer.h"
#include "services/local_events.h"

#include <functional>

namespace events {
class EventManager;
}

class Profile;

struct EventsHelperContext {
  events::EventManager& event_manager_;
  LocalEvents& local_events_;
  Profile& profile_;
  const std::function<void(bool has_events)> events_handler_;
};

class EventsHelper final : private EventsHelperContext,
                           private events::EventObserver,
                           private LocalEvents::Observer {
 public:
  explicit EventsHelper(EventsHelperContext&& context);
  ~EventsHelper();

 private:
  void ShowEvents(bool added, bool delayed);

  // events::EventObserver
  virtual void OnEventReported(const scada::Event& event) override;
  virtual void OnEventAcknowledged(const scada::Event& event) override;
  virtual void OnAllEventsAcknowledged() override;

  // LocalEvents::Observer
  virtual void OnLocalEvent(const scada::Event& event) override;

  bool playing_alarm_sound_ = false;

  bool showing_events_ = false;
  bool showing_events_added_ = false;

  base::WeakPtrFactory<EventsHelper> weak_factory_{this};
};
