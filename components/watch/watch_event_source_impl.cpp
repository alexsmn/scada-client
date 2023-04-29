#include "components/watch/watch_event_source_impl.h"

#include "core/monitored_item_service.h"
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
  monitored_item_.reset();

  monitored_item_ =
      node_service_.GetNode(scada::id::Server)
          .CreateMonitoredItem(
              scada::AttributeId::EventNotifier,
              scada::MonitoringParameters{}.set_filter(
                  scada::EventFilter{}
                      .add_of_type(devices::id::DeviceWatchEventType)
                      .add_child_of(device_id)));

  if (!monitored_item_)
    return delegate_->OnError(scada::StatusCode::Bad);

  // FIXME: Captures |this|. No sync.
  monitored_item_->Subscribe(
      [this](const scada::Status& status, const std::any& event) {
        if (!status)
          return delegate_->OnError(status);

        if (event.has_value()) {
          auto* system_event = std::any_cast<scada::Event>(&event);
          assert(system_event);
          if (system_event)
            delegate_->OnEvent(*system_event);
        }
      });
}
