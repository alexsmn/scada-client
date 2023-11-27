#include "components/watch/watch_history_event_source.h"

#include "base/executor.h"
#include "node_service/node_service.h"

// WatchHistoryEventSource

WatchHistoryEventSource::WatchHistoryEventSource(
    WatchHistorySourceContext&& context)
    : WatchHistorySourceContext{std::move(context)} {}

void WatchHistoryEventSource::SetDelegate(Delegate* delegate) {
  delegate_ = delegate;
}

void WatchHistoryEventSource::SetDeviceId(const scada::NodeId& device_id) {
  device_id_ = device_id;

  FetchEvents();
}

void WatchHistoryEventSource::SetTimeRange(const TimeRange& time_range) {
  time_range_ = time_range;

  FetchEvents();
}

void WatchHistoryEventSource::FetchEvents() {
  assert(delegate_);

  cancelation_.Cancel();

  if (device_id_.is_null()) {
    return;
  }

  node_service_.GetNode(device_id_)
      .scada_node()
      .read_event_history()
      .then(BindExecutor(executor_, cancelation_,
                         [this](const std::vector<scada::Event>& events) {
                           for (const auto& event : events) {
                             delegate_->OnEvent(event);
                           }
                         }));
}
