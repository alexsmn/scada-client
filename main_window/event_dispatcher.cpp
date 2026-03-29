#include "main_window/event_dispatcher.h"

#include "base/executor.h"
#include "common_resources.h"
#include "events/local_events.h"
#include "events/node_event_provider.h"
#include "main_window/action_manager.h"
#include "profile/profile.h"

#include <mmsystem.h>

using namespace std::chrono_literals;
namespace {
const auto kDelay = 300ms;
}

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
    executor_->PostDelayedTask(
      kDelay,
      cancelation_.Bind([this] { ShowEvents(showing_events_added_); }));
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

  bool play_sound = has_events && profile_.event_play_sound;
  if (playing_alarm_sound_ != play_sound) {
    playing_alarm_sound_ = play_sound;

    if (playing_alarm_sound_)
      PlaySound((LPCTSTR)SND_ALIAS_SYSTEMEXCLAMATION, nullptr,
                SND_ALIAS_ID | SND_ASYNC | SND_LOOP);
    else
      PlaySound(nullptr, nullptr, 0);
  }
}
