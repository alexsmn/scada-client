#include "components/watch/watch_history_event_source.h"

#include "node_service/node_service.h"

// WatchHistoryEventSource

WatchHistoryEventSource::WatchHistoryEventSource(
    WatchHistorySourceContext&& context)
    : WatchHistorySourceContext{std::move(context)} {}

void WatchHistoryEventSource::SetDelegate(Delegate* delegate) {
  delegate_ = delegate;
}

void WatchHistoryEventSource::SetDeviceId(const scada::NodeId& device_id) {
  assert(delegate_);

  node_service_.GetNode(device_id).scada_node().read_event_history().then(
      [this](const std::vector<scada::Event>& events) {
        for (const auto& event : events) {
          delegate_->OnEvent(event);
        }
      });
}
