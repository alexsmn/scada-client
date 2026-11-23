#include "main_window/status_bar/event_status_provider.h"

#include "aui/translation.h"
#include "base/u16format.h"
#include "events/alarm_set.h"
#include "events/local_events.h"
#include "events/node_event_provider.h"
#include "profile/profile.h"
#include "scada/event.h"

void EventStatusProvider::Init(const ChangeNotifier& change_notifier) {
  change_notifier_ = change_notifier;

  node_event_provider_.AddObserver(*this);

  connections_.emplace_back(profile_.AddChangeObserver(change_notifier));

  connections_.emplace_back(local_events_.event_signal().connect(
      [change_notifier](const scada::Event&) { change_notifier(); }));
}

EventStatusProvider::~EventStatusProvider() {
  node_event_provider_.RemoveObserver(*this);
}

events::SeverityTileCounts EventStatusProvider::GetTileCounts() const {
  // The client's event model keeps every current alarm in its ISA-18.2 state —
  // unacknowledged, acknowledged but still standing, or cleared unread
  // (backlog 1117) — and the tiles count active ones by severity and unread
  // ones as Unacknowledged. The set is CollectAlarms', the same one the
  // escalation ladder reads, so a tone never sounds over tiles that show
  // nothing (backlog 888).
  return events::CountSeverityTiles(
      events::CollectAlarms(node_event_provider_, local_events_));
}

int EventStatusProvider::GetAlarmCount() const {
  return GetTileCounts().unacknowledged;
}

int EventStatusProvider::GetSeverityCount(
    scada::aui::SeverityLevel level) const {
  const events::SeverityTileCounts counts = GetTileCounts();
  switch (level) {
    case scada::aui::SeverityLevel::kCritical:
      return counts.critical;
    case scada::aui::SeverityLevel::kWarning:
      return counts.warning;
    case scada::aui::SeverityLevel::kNone:
      return 0;
  }
  return 0;
}

std::u16string EventStatusProvider::GetPanelFilterText() const {
  // The events panel's threshold, which hides rows from that panel only
  // (backlog 889); the fetcher behind the top-bar tiles is never narrowed.
  // Empty at the scale's floor, which matches every event: an unfiltered
  // panel needs no reminder. Worded as a filter rather than «Важность: N»,
  // which read as a count beside the alarm cells it used to sit next to
  // (backlog 1122).
  if (profile_.event_panel_severity_min <= scada::kSeverityMin) {
    return {};
  }
  return u16format(Translate("Panel filter: severity ≥ {}"),
                   profile_.event_panel_severity_min);
}

void EventStatusProvider::OnEvents(
    std::span<const scada::Event* const> events) {
  change_notifier_();
}