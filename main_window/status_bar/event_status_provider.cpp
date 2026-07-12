#include "main_window/status_bar/event_status_provider.h"

#include "aui/translation.h"
#include "base/u16format.h"
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

  return event_count != 0
             ? u16format(L"\u0421\u043e\u0431\u044b\u0442\u0438\u044f: {}",
                         event_count)
             : u"\u041d\u0435\u0442 \u0441\u043e\u0431\u044b\u0442\u0438\u0439";
}

int EventStatusProvider::GetAlarmCount() const {
  return static_cast<int>(node_event_provider_.unacked_events().size());
}

int EventStatusProvider::GetSeverityCount(aui::SeverityLevel level) const {
  int count = 0;
  for (const scada::Event& event :
       node_event_provider_.unacked_events() | std::views::values) {
    if (SeverityLevelForEvent(event.severity) == level)
      ++count;
  }
  return count;
}

std::u16string EventStatusProvider::GetSeverityText() const {
  return u16format(L"\u0412\u0430\u0436\u043d\u043e\u0441\u0442\u044c: {}",
                   node_event_provider_.severity_min());
}

aui::SeverityLevel SeverityLevelForEvent(unsigned severity) {
  if (severity >= scada::kSeverityCritical)
    return aui::SeverityLevel::kCritical;
  if (severity >= scada::kSeverityWarning)
    return aui::SeverityLevel::kWarning;
  return aui::SeverityLevel::kNone;
}

aui::SeverityLevel EventStatusProvider::HighestUnackedLevel() const {
  scada::UInt32 highest = 0;
  for (const scada::Event& event :
       node_event_provider_.unacked_events() | std::views::values) {
    highest = std::max(highest, event.severity);
  }
  return SeverityLevelForEvent(highest);
}

std::u16string EventStatusProvider::GetHighestSeverityText() const {
  // Keep the legacy status bar untouched: the coloured highest-severity cell is
  // part of the opt-in token themes only.
  if (aui::GetSeverityTheme() == aui::SeverityTheme::kLegacy)
    return {};

  // English literals routed through Translate(); the Russian (and any other
  // language) lives in the .ts — never hardcode localized text here.
  switch (HighestUnackedLevel()) {
    case aui::SeverityLevel::kCritical:
      return Translate("Critical");
    case aui::SeverityLevel::kWarning:
      return Translate("Warning");
    case aui::SeverityLevel::kNone:
      return {};  // calm: no active alarm, show nothing
  }
  return {};
}

std::optional<aui::Color> EventStatusProvider::GetHighestSeverityColor() const {
  return aui::SeverityColor(HighestUnackedLevel());
}

void EventStatusProvider::OnEvents(
    std::span<const scada::Event* const> events) {
  change_notifier_();
}