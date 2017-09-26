#include "components/main/events_helper.h"

#include "base/bind.h"
#include "base/threading/thread_task_runner_handle.h"
#include "services/local_events.h"
#include "components/main/main_window.h"
#include "services/profile.h"
#include "common/event_manager.h"

#include <MMSystem.h>

EventsHelper::EventsHelper(EventsHelperContext&& context)
    : EventsHelperContext{std::move(context)} {
  event_manager_.AddObserver(*this);

  ShowEvents(true, true);
}

EventsHelper::~EventsHelper() {
  event_manager_.RemoveObserver(*this);
}

void EventsHelper::OnEventReported(const scada::Event& event) {
  ShowEvents(true, true);
}

void EventsHelper::OnEventAcknowledged(const scada::Event& event) {
  ShowEvents(false, true);
}

void EventsHelper::OnAllEventsAcknowledged() {
  ShowEvents(false, true);
}

void EventsHelper::ShowEvents(bool added, bool delayed) {
  if (delayed) {
    if (!showing_events_) {
      showing_events_ = true;
      base::ThreadTaskRunnerHandle::Get()->PostTask(FROM_HERE, base::Bind(
          &EventsHelper::ShowEvents, weak_factory_.GetWeakPtr(),
          base::ConstRef(showing_events_added_), false));
    }
    showing_events_added_ = added;
    return;
  }

  showing_events_ = false;
  
  bool has_events = !event_manager_.unacked_events().empty() ||
                    !local_events_.events().empty();
  
  // Never show window if event removed.
  if (has_events && !added)
    return;

  events_handler_(has_events);

  bool play_sound = has_events && profile_.event_play_sound;
  if (playing_alarm_sound_ != play_sound) {
    playing_alarm_sound_ = play_sound;

    if (playing_alarm_sound_)
      PlaySound((LPCTSTR)SND_ALIAS_SYSTEMEXCLAMATION, nullptr, SND_ALIAS_ID|SND_ASYNC|SND_LOOP);
    else
      PlaySound(nullptr, nullptr, 0);
  }
}
