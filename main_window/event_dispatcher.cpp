#include "main_window/event_dispatcher.h"

#include "aui/translation.h"
#include "base/any_executor_dispatch.h"
#include "controller/action_manager.h"
#include "events/event_severity.h"
#include "events/local_events.h"
#include "events/node_event_provider.h"
#include "events/severity_tiles.h"
#include "profile/profile.h"
#include "resources/common_resources.h"
#include "services/speech_service.h"

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

events::AlarmEscalation EventDispatcher::CurrentEscalation() const {
  // Reduced through CountSeverityTiles rather than by counting here, so this
  // and the context bar's tiles cannot disagree about the same alarms — the
  // reason EscalationFor takes tile counts in the first place.
  std::vector<events::AlarmSummary> alarms;
  alarms.reserve(node_event_provider_.unacked_events().size() +
                 local_events_.events().size());
  auto account = [&alarms](const scada::Event& event) {
    alarms.push_back(events::AlarmSummary{
        .severity = events::SeverityLevelForEvent(event.severity),
        .acknowledged = event.acked,
    });
  };
  for (const auto& [event_id, event] : node_event_provider_.unacked_events())
    account(event);
  for (const scada::Event* event : local_events_.events())
    account(*event);

  return events::EscalationFor(events::CountSeverityTiles(alarms));
}

void EventDispatcher::ShowEvents(bool added) {
  showing_events_ = false;

  bool has_events = !node_event_provider_.unacked_events().empty() ||
                    !local_events_.events().empty();

  if (has_events != has_events_) {
    has_events_ = has_events;
    action_manager_.NotifyActionChanged(ID_ACKNOWLEDGE_ALL);
  }

  // Never show the window when the dispatch was a removal. This used to be an
  // early return, which the annunciators below sat behind — and once they gate
  // on the ladder rather than on `has_events` that is wrong: acknowledging the
  // last unacknowledged critical while warnings stand lowers the ladder without
  // lowering `has_events`, so the tone would go on sounding for an alarm state
  // that no longer escalates (backlog 552).
  if (!has_events || added)
    events_handler_(has_events);

  // Both annunciators gate on the escalation ladder, not on the bare "something
  // is unacknowledged" edge: an unacknowledged critical, or a flood. That is
  // what the web client has always gated its tone on, and a plant that
  // escalates at different moments depending on which client is open is the
  // defect events/alarm_escalation.h exists to prevent. Every unacknowledged
  // event still reaches the operator — the status-bar count, the journal and
  // the context bar's tiles are unchanged; what the ladder decides is when the
  // room is made to *sound*.
  const bool escalated = CurrentEscalation().escalated();

  // The audible annunciator. The latch is platform-independent so that the
  // option means the same thing everywhere; only the emission below differs.
  bool play_sound = escalated && profile_.event_play_sound;
  if (playing_alarm_sound_ != play_sound) {
    playing_alarm_sound_ = play_sound;

    if (alarm_sound_handler_)
      alarm_sound_handler_(playing_alarm_sound_);
    else
      PlayAlarmSound(playing_alarm_sound_);
  }

  // The spoken announcement takes the same *escalation* edge, but a latch of
  // its own: «Speech» and «Sound Alarm on Event» are separate
  // options, and folding them together would leave speech silent whenever the
  // tone was switched off. It speaks only as the alarm arrives — there is
  // nothing to say once the last event is acknowledged, and repeating it on
  // every dispatch would talk over the operator. Whether there is a voice at
  // all is the service's to answer; there is none on a non-Windows build.
  if (announced_alarm_ != escalated) {
    announced_alarm_ = escalated;

    if (announced_alarm_ && profile_.speech_enabled && speech_service_ &&
        speech_service_->is_ok()) {
      speech_service_->Speak(Translate("Unacknowledged alarm"));
    }
  }
}
