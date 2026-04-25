#include "components/watch/watch_history_event_source.h"

#include "base/awaitable_promise.h"
#include "base/executor.h"
#include "net/net_executor_adapter.h"
#include "node_service/node_service.h"

namespace {

scada::DateTime SanitizeTimeBound(scada::DateTime time) {
  return time.is_min() || time.is_max() ? scada::DateTime{} : time;
}

Awaitable<void> ReadHistoryEventsAsync(std::shared_ptr<Executor> executor,
                                       NodeRef device,
                                       scada::DateTimeRange time_range,
                                       CancelationRef cancelation,
                                       WatchEventSource::Delegate& delegate) {
  try {
    auto events = co_await AwaitPromise(
        NetExecutorAdapter{executor},
        device.scada_node().read_event_history(
            {.from = SanitizeTimeBound(time_range.first),
             .to = SanitizeTimeBound(time_range.second)}));

    if (cancelation.canceled()) {
      co_return;
    }

    for (const auto& event : events) {
      delegate.OnEvent(event);
    }
  } catch (...) {
  }

  co_return;
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

  CoSpawn(executor_,
          [executor = executor_, device = node_service_.GetNode(device_id),
           time_range, cancelation = cancelation_.ref(),
           &delegate]() mutable -> Awaitable<void> {
            co_await ReadHistoryEventsAsync(executor, device, time_range,
                                            cancelation, delegate);
          });
}
