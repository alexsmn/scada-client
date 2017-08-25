#pragma once

#include "base/memory/weak_ptr.h"
#include "common/event_observer.h"

namespace events {
class EventManager;
}

class LocalEvents;
class MainWindow;
class Profile;

class EventsHelper : private events::EventObserver {
 public:
  EventsHelper(MainWindow& main_window, events::EventManager& event_manager, LocalEvents& local_events, Profile& profile);
  ~EventsHelper();

 private:
  void ShowEvents(bool added, bool delayed);

  // events::EventObserver
  virtual void OnEventReported(const scada::Event& event) override;
  virtual void OnEventAcknowledged(const scada::Event& event) override;
  virtual void OnAllEventsAcknowledged() override;

  MainWindow& main_window_;
  events::EventManager& event_manager_;
  LocalEvents& local_events_;
  Profile& profile_;

  bool playing_alarm_sound_ = false;

  bool showing_events_ = false;
  bool showing_events_added_ = false;

  base::WeakPtrFactory<EventsHelper> weak_factory_;
};