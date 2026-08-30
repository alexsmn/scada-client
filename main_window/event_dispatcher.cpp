#include "main_window/event_dispatcher.h"

#include "base/any_executor_dispatch.h"
#include "controller/action_manager.h"
#include "events/local_events.h"
#include "events/node_event_provider.h"
#include "profile/profile.h"
#include "resources/common_resources.h"

#if defined(_WIN32)
#include <mmsystem.h>
#else
#include <QApplication>
#endif

namespace {

// The platform's audible annunciator. Windows loops a system alias for as long
// as the alarm stands, so it takes both edges; everywhere else the tone is
// one-shot on the rising edge, because Qt Widgets offers no looping system
// sound — `QSoundEffect` lives in Qt Multimedia, which this client does not
// depend on. So on those platforms there is nothing to stop on the falling
// edge, and the annunciation that lasts until acknowledgement remains the
// status-bar count rather than the tone.
void PlayAlarmSound(bool playing) {
#if defined(_WIN32)
  if (playing)
    PlaySound((LPCTSTR)SND_ALIAS_SYSTEMEXCLAMATION, nullptr,
              SND_ALIAS_ID | SND_ASYNC | SND_LOOP);
  else
    PlaySound(nullptr, nullptr, 0);
#else
  if (playing)
    QApplication::beep();
#endif
}

}  // namespace

EventDispatcher::EventDispatcher(EventDispatcherContext&& context)
    : EventDispatcherContext{std::move(context)} {
  node_event_provider_.AddObserver(*this);

  local_event_connection_ = local_events_.event_signal().connect(
      [this](const scada::Event& event) { ShowEventsDelayed(!event.acked); });

  ShowEventsDelayed(true);
}

EventDispatcher::~EventDispatcher() {
  node_event_provider_.RemoveObserver(*this);
}

void EventDispatcher::OnEvents(std::span<const scada::Event* const> events) {
  bool all_acked = std::ranges::all_of(
      events, [](const scada::Event* event) { return event->acked; });
  ShowEventsDelayed(!all_acked);
}

void EventDispatcher::OnAllEventsAcknowledged() {
  ShowEventsDelayed(false);
}

void EventDispatcher::ShowEventsDelayed(bool added) {
  if (!showing_events_) {
    showing_events_ = true;
    PostDelayedTask(executor_, event_debounce_, cancelation_.Bind([this] {
      ShowEvents(showing_events_added_);
    }));
  }
  showing_events_added_ = added;
}

void EventDispatcher::ShowEvents(bool added) {
  showing_events_ = false;

  bool has_events = !node_event_provider_.unacked_events().empty() ||
                    !local_events_.events().empty();

  if (has_events != has_events_) {
    has_events_ = has_events;
    action_manager_.NotifyActionChanged(ID_ACKNOWLEDGE_ALL);
  }

  // Never show window if event removed.
  if (has_events && !added)
    return;

  events_handler_(has_events);

  // The audible annunciator. The latch is platform-independent so that the
  // option means the same thing everywhere; only the emission below differs.
  bool play_sound = has_events && profile_.event_play_sound;
  if (playing_alarm_sound_ != play_sound) {
    playing_alarm_sound_ = play_sound;

    if (alarm_sound_handler_)
      alarm_sound_handler_(playing_alarm_sound_);
    else
      PlayAlarmSound(playing_alarm_sound_);
  }
}
