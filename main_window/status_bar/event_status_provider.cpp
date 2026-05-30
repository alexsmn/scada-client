#include "main_window/status_bar/event_status_provider.h"

#include "base/u16format.h"
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

  return event_count != 0 ? u16format(L"\u0421\u043e\u0431\u044b\u0442\u0438\u044f: {}", event_count)
                          : u"\u041d\u0435\u0442 \u0441\u043e\u0431\u044b\u0442\u0438\u0439";
}

std::u16string EventStatusProvider::GetSeverityText() const {
  return u16format(L"\u0412\u0430\u0436\u043d\u043e\u0441\u0442\u044c: {}",
                            node_event_provider_.severity_min());
}

void EventStatusProvider::OnEvents(
    std::span<const scada::Event* const> events) {
  change_notifier_();
}