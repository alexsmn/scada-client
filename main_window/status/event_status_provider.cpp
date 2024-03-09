#include "main_window/status/event_status_provider.h"

#include "base/strings/stringprintf.h"
#include "events/node_event_provider.h"
#include "profile/profile.h"

void EventStatusProvider::Init(const ChangeNotifier& change_notifier) {
  change_notifier_ = change_notifier;

  connection_ = profile_.AddChangeObserver(change_notifier);
}

std::u16string EventStatusProvider::GetEventCountText() const {
  size_t unacked_event_count = node_event_provider_.unacked_events().size();
  return unacked_event_count
             ? base::StringPrintf(u"Событий: %u", unacked_event_count)
             : u"Нет событий";
}

std::u16string EventStatusProvider::GetSeverityText() const {
  return base::StringPrintf(u"Важность: %u",
                            node_event_provider_.severity_min());
}