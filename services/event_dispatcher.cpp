#include "services/event_dispatcher.h"

#include "base/executor.h"
#include "common/event_fetcher.h"
#include "common_resources.h"
#include "components/main/action_manager.h"
#include "services/profile.h"

#include <MMSystem.h>

using namespace std::chrono_literals;
namespace {
const auto kDelay = 300ms;
}

EventDispatcher::EventDispatcher(EventDispatcherContext&& context)
    : EventDispatcherContext{std::move(context)} {
  event_fetcher_.AddObserver(*this);
  local_events_.observers().AddObserver(this);

  ShowEventsDelayed(true);
}

EventDispatcher::~EventDispatcher() {
  local_events_.observers().RemoveObserver(this);
  event_fetcher_.RemoveObserver(*this);
}

void EventDispatcher::OnEventReported(const scada::Event& event) {
  ShowEventsDelayed(true);
}

void EventDispatcher::OnEventAcknowledged(const scada::Event& event) {
  ShowEventsDelayed(false);
}

void EventDispatcher::OnAllEventsAcknowledged() {
  ShowEventsDelayed(false);
}

void EventDispatcher::ShowEventsDelayed(bool added) {
  if (!showing_events_) {
    showing_events_ = true;
    executor_->PostDelayedTask(kDelay,
                               [this, weak_ptr = weak_factory_.GetWeakPtr()] {
                                 if (weak_ptr.get())
                                   ShowEvents(showing_events_added_);
                               });
  }
  showing_events_added_ = added;
}

void EventDispatcher::ShowEvents(bool added) {
  showing_events_ = false;

  bool has_events = !event_fetcher_.unacked_events().empty() ||
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

void EventDispatcher::OnLocalEvent(const scada::Event& event) {
  ShowEventsDelayed(!event.acked);
}
