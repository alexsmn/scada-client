#include "main_window/status_bar/event_status_provider.h"

#include "aui/translation.h"
#include "base/u16format.h"
#include "events/alarm_set.h"
#include "events/local_events.h"
#include "events/node_event_provider.h"
#include "profile/profile.h"
#include "scada/event.h"

#include <algorithm>
#include <ranges>

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

std::u16string EventStatusProvider::GetEventCountText() const {
  size_t event_count = node_event_provider_.unacked_events().size() +
                       local_events_.events().size();

  return event_count != 0 ? u16format(Translate("Events: {}"), event_count)
                          : Translate("No events");
}

events::SeverityTileCounts EventStatusProvider::GetTileCounts() const {
  // The client's event model retains only alarms nobody has acknowledged yet
  // (NodeEventProvider drops an event once it is acked) and carries no
  // condition-cleared state, so every alarm it knows about is active and
  // unacknowledged. The set is CollectAlarms', the same one the escalation
  // ladder reads, so a tone never sounds over tiles that show nothing
  // (backlog 888).
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

std::u16string EventStatusProvider::GetSeverityText() const {
  // The events panel's threshold, which hides rows from that panel only
  // (backlog 889); the fetcher behind the counts beside it is never narrowed.
  return u16format(Translate("Severity: {}"),
                   profile_.event_panel_severity_min);
}

scada::aui::SeverityLevel EventStatusProvider::HighestUnackedLevel() const {
  scada::UInt32 highest = 0;
  for (const scada::Event& event :
       node_event_provider_.unacked_events() | std::views::values) {
    highest = std::max(highest, event.severity);
  }
  return events::SeverityLevelForEvent(highest);
}

std::u16string EventStatusProvider::GetHighestSeverityText() const {
  // Empty when calm: no active alarm, nothing to shout about.
  return events::SeverityLevelLabel(HighestUnackedLevel());
}

std::optional<scada::aui::Color> EventStatusProvider::GetHighestSeverityColor()
    const {
  return scada::aui::SeverityColor(HighestUnackedLevel());
}

void EventStatusProvider::OnEvents(
    std::span<const scada::Event* const> events) {
  change_notifier_();
}