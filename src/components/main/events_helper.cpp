#include "client/components/main/events_helper.h"

#include "base/bind.h"
#include "base/threading/thread_task_runner_handle.h"
#include "client/common_resources.h"
#include "client/services/local_events.h"
#include "client/components/main/main_window.h"
#include "client/services/profile.h"
#include "common/event_manager.h"

#include <MMSystem.h>

EventsHelper::EventsHelper(MainWindow& main_window, events::EventManager& event_manager, LocalEvents& local_events, Profile& profile)
    : main_window_(main_window),
      event_manager_(event_manager),
      local_events_(local_events),
      profile_(profile),
      weak_factory_(this) {
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

  bool events_shown = main_window_.FindOpenedViewByType(ID_EVENT_VIEW) != nullptr;
  if (has_events != events_shown) {
    if (has_events && profile_.event_auto_show)
      main_window_.OpenPane(ID_EVENT_VIEW, false);
    else if (!has_events && profile_.event_auto_hide)
      main_window_.ClosePane(ID_EVENT_VIEW);
  }

  main_window_.SetWindowFlashing(has_events && profile_.event_flash_window);

  bool play_sound = has_events && profile_.event_play_sound;
  if (playing_alarm_sound_ != play_sound) {
    playing_alarm_sound_ = play_sound;

    if (playing_alarm_sound_)
      PlaySound((LPCTSTR)SND_ALIAS_SYSTEMEXCLAMATION, nullptr, SND_ALIAS_ID|SND_ASYNC|SND_LOOP);
    else
      PlaySound(nullptr, nullptr, 0);
  }
}
