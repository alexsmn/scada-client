#include "components/watch/watch_event_source_impl.h"

#include "model/devices_node_ids.h"
#include "node_service/node_service.h"

// WatchEventSourceImpl

WatchEventSourceImpl::WatchEventSourceImpl(
    WatchEventSourceImplContext&& context)
    : WatchEventSourceImplContext{std::move(context)} {}

void WatchEventSourceImpl::SetDelegate(Delegate* delegate) {
  delegate_ = delegate;
}

void WatchEventSourceImpl::SetDeviceId(const scada::NodeId& device_id) {
  monitored_item_.unsubscribe();

  monitored_item_.subscribe_system_events(
      node_service_.GetNode(device_id).scada_node(),
      // FIXME: MSVC fails with an internal error if named parameters are used
      // with variants.
      scada::MonitoringParameters{}.set_filter(
          scada::EventFilter{.of_type = {devices::id::DeviceWatchEventType}}),
      // FIXME: Captures |this|. No sync.
      [this](const scada::Status& status, const scada::Event& event) {
        if (status) {
          delegate_->OnError(status);
          return;
        }
        delegate_->OnEvent(event);
      });
}
