#include "modules/watch/watch_current_event_source.h"

#include "base/any_executor_dispatch.h"
#include "model/devices_node_ids.h"
#include "node_service/node_service.h"
#include "scada/monitoring_parameters.h"

#include <any>

// WatchCurrentEventSource

WatchCurrentEventSource::WatchCurrentEventSource(
    WatchCurrentEventSourceContext&& context)
    : WatchCurrentEventSourceContext{std::move(context)} {}

void WatchCurrentEventSource::Start(const scada::NodeId& device_id,
                                    const scada::TimeRange& time_range,
                                    Delegate& delegate) {
  monitored_item_.unsubscribe();

  if (time_range.second != scada::kMaxTime) {
    return;
  }

  // Subscribes the any-typed stream rather than subscribe_system_events, which
  // any_casts to scada::Event and would silently drop a DeviceFrameEvent
  // entirely — not merely its decoded fields, but the log line with it. The
  // filter still names DeviceWatchEventType: DeviceFrameEventType subtypes it,
  // so frames arrive without widening the subscription.
  monitored_item_.subscribe_events(
      node_service_.GetNode(device_id).scada_node(),
      scada::MonitoringParameters{
          .filter =
              scada::EventFilter{
                  .of_type = {scada::devices::id::DeviceWatchEventType}}},
      // FIXME: Captures |this|. No sync.
      BindExecutor(executor_, [&delegate](const scada::Status& status,
                                          const std::any& event) {
        if (!status) {
          delegate.OnError(status);
          return;
        }
        if (const auto* frame = std::any_cast<scada::DeviceFrameEvent>(&event)) {
          delegate.OnDeviceFrame(*frame);
        } else if (const auto* base = std::any_cast<scada::Event>(&event)) {
          delegate.OnEvent(*base);
        }
      }));
}
