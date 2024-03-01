#include "components/watch/watch_current_event_source.h"

#include "base/executor.h"
#include "model/devices_node_ids.h"
#include "node_service/node_service.h"
#include "scada/monitoring_parameters.h"

// WatchCurrentEventSource

WatchCurrentEventSource::WatchCurrentEventSource(
    WatchCurrentEventSourceContext&& context)
    : WatchCurrentEventSourceContext{std::move(context)} {}

void WatchCurrentEventSource::Start(const scada::NodeId& device_id,
                                    const scada::DateTimeRange& time_range,
                                    Delegate& delegate) {
  monitored_item_.unsubscribe();

  if (!time_range.second.is_max()) {
    return;
  }

  monitored_item_.subscribe_system_events(
      node_service_.GetNode(device_id).scada_node(),
      scada::MonitoringParameters{
          .filter =
              scada::EventFilter{
                  .of_type = {devices::id::DeviceWatchEventType}}},
      // FIXME: Captures |this|. No sync.
      BindExecutor(executor_, [&delegate](const scada::Status& status,
                                          const scada::Event& event) {
        if (!status) {
          delegate.OnError(status);
          return;
        }
        delegate.OnEvent(event);
      }));
}
