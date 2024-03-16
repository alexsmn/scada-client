#include "main_window/status/event_status_provider.h"

#include "base/strings/stringprintf.h"
#include "events/node_event_provider.h"
#include "profile/profile.h"
#include "events/local_events.h"

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

  return event_count != 0 ? base::StringPrintf(u"Событий: %u", event_count)
                          : u"Нет событий";
}

std::u16string EventStatusProvider::GetSeverityText() const {
  return base::StringPrintf(u"Важность: %u",
                            node_event_provider_.severity_min());
}

void EventStatusProvider::OnEvents(
    std::span<const scada::Event* const> events) {
  change_notifier_();
}