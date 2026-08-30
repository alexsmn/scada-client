#pragma once

#include "base/any_executor.h"

#include "base/cancelation.h"
#include "events/event_observer.h"

#include <boost/signals2/connection.hpp>
#include <chrono>
#include <functional>

class ActionManager;
class LocalEvents;
class NodeEventProvider;
class Profile;
class SpeechService;

// How long a burst of arriving events is coalesced before the window is shown
// and the annunciators fire, so that a flood produces one announcement rather
// than one per event.
inline constexpr auto kDefaultEventDebounce = std::chrono::milliseconds{300};

struct EventDispatcherContext {
  const AnyExecutor executor_;
  NodeEventProvider& node_event_provider_;
  LocalEvents& local_events_;
  Profile& profile_;
  const std::function<void(bool has_events)> events_handler_;
  ActionManager& action_manager_;
  // Emits the audible annunciator, and is left empty in production — where the
  // platform tone in `event_dispatcher.cpp` is used instead. Tests inject a
  // handler so that the suite stays silent and can assert that the tone was
  // asked for, which is as close to the platform as a test can get.
  const std::function<void(bool playing)> alarm_sound_handler_;
  // Spoken announcements, behind «Speech». Null where the client runs without
  // one; the service is additionally inert unless `is_ok()`, which is false on
  // every non-Windows build, so both have to be checked before the option can
  // be said to have announced anything.
  SpeechService* const speech_service_ = nullptr;
  // The debounce above, overridable so that tests do not have to wait out a
  // real timer to observe an announcement.
  const std::chrono::nanoseconds event_debounce_ = kDefaultEventDebounce;
};

class EventDispatcher final : private EventDispatcherContext,
                              private EventObserver {
 public:
  explicit EventDispatcher(EventDispatcherContext&& context);
  ~EventDispatcher();

  // Whether the audible annunciator is currently asked for. Mirrors the
  // requested state rather than the platform's, which neither `PlaySound` nor
  // `QApplication::beep` reports back, and is what the regression tests assert.
  bool playing_alarm_sound() const { return playing_alarm_sound_; }

 private:
  void ShowEventsDelayed(bool added);
  void ShowEvents(bool added);

  // EventObserver
  virtual void OnEvents(std::span<const scada::Event* const> events) override;
  virtual void OnAllEventsAcknowledged() override;

  bool playing_alarm_sound_ = false;
  bool announced_alarm_ = false;

  bool has_events_ = false;
  bool showing_events_ = false;
  bool showing_events_added_ = false;

  boost::signals2::scoped_connection local_event_connection_;

  Cancelation cancelation_;
};
