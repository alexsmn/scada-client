#include "services/event_notifier.h"

#include "base/bind.h"
#include "base/threading/thread_task_runner_handle.h"
#include "common/event_fetcher.h"
#include "common_resources.h"
#include "components/main/action_manager.h"
#include "services/profile.h"

#include <MMSystem.h>

namespace {
const auto kDelay = base::TimeDelta::FromMilliseconds(300);
}

EventNotifier::EventNotifier(EventNotifierContext&& context)
    : EventNotifierContext{std::move(context)} {
  event_fetcher_.AddObserver(*this);
  local_events_.observers().AddObserver(this);

  ShowEventsDelayed(true);
}

EventNotifier::~EventNotifier() {
  local_events_.observers().RemoveObserver(this);
  event_fetcher_.RemoveObserver(*this);
}

void EventNotifier::OnEventReported(const scada::Event& event) {
  ShowEventsDelayed(true);
}

void EventNotifier::OnEventAcknowledged(const scada::Event& event) {
  ShowEventsDelayed(false);
}

void EventNotifier::OnAllEventsAcknowledged() {
  ShowEventsDelayed(false);
}

void EventNotifier::ShowEventsDelayed(bool added) {
  if (!showing_events_) {
    showing_events_ = true;
    base::ThreadTaskRunnerHandle::Get()->PostDelayedTask(
        FROM_HERE,
        base::Bind(&EventNotifier::ShowEvents, weak_factory_.GetWeakPtr(),
                   base::ConstRef(showing_events_added_)),
        kDelay);
  }
  showing_events_added_ = added;
}

void EventNotifier::ShowEvents(bool added) {
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

void EventNotifier::OnLocalEvent(const scada::Event& event) {
  ShowEventsDelayed(!event.acked);
}
