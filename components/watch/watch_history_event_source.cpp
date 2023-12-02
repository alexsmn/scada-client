#include "components/watch/watch_history_event_source.h"

#include "base/executor.h"
#include "node_service/node_service.h"

namespace {

scada::DateTime SanitizeTimeBound(scada::DateTime time) {
  return time.is_min() || time.is_max() ? scada::DateTime{} : time;
}

}  // namespace

// WatchHistoryEventSource

WatchHistoryEventSource::WatchHistoryEventSource(
    WatchHistorySourceContext&& context)
    : WatchHistorySourceContext{std::move(context)} {}

void WatchHistoryEventSource::Start(const scada::NodeId& device_id,
                                    const scada::DateTimeRange& time_range,
                                    Delegate& delegate) {
  cancelation_.Cancel();

  if (device_id.is_null() || time_range.first.is_max()) {
    return;
  }

  node_service_.GetNode(device_id)
      .scada_node()
      .read_event_history({.from = SanitizeTimeBound(time_range.first),
                           .to = SanitizeTimeBound(time_range.second)})
      .then(BindExecutor(executor_, cancelation_,
                         [&delegate](const std::vector<scada::Event>& events) {
                           for (const auto& event : events) {
                             delegate.OnEvent(event);
                           }
                         }));
}
