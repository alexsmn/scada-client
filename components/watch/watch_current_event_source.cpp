#include "components/watch/watch_current_event_source.h"

#include "model/devices_node_ids.h"
#include "node_service/node_service.h"

// WatchCurrentEventSource

WatchCurrentEventSource::WatchCurrentEventSource(
    WatchCurrentEventSourceContext&& context)
    : WatchCurrentEventSourceContext{std::move(context)} {}

void WatchCurrentEventSource::SetDelegate(Delegate* delegate) {
  delegate_ = delegate;
}

void WatchCurrentEventSource::SetDeviceId(const scada::NodeId& device_id) {
  assert(delegate_);

  monitored_item_.unsubscribe();

  monitored_item_.subscribe_system_events(
      node_service_.GetNode(device_id).scada_node(),
      // FIXME: MSVC fails with an internal error if named parameters are used
      // with variants.
      scada::MonitoringParameters{}.set_filter(
          scada::EventFilter{.of_type = {devices::id::DeviceWatchEventType}}),
      // FIXME: Captures |this|. No sync.
      [this](const scada::Status& status, const scada::Event& event) {
        if (!status) {
          delegate_->OnError(status);
          return;
        }
        delegate_->OnEvent(event);
      });
}
