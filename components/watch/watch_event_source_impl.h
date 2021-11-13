#pragma once

#include "components/watch/watch_event_source.h"

#include <memory>

namespace scada {
class MonitoredItem;
}  // namespace scada

class NodeService;

struct WatchEventSourceImplContext {
  NodeService& node_service_;
};

class WatchEventSourceImpl : private WatchEventSourceImplContext,
                             public WatchEventSource {
 public:
  explicit WatchEventSourceImpl(WatchEventSourceImplContext&& context);

  // WatchEventSource
  virtual void SetDelegate(Delegate* delegate) override;
  virtual void SetDeviceId(const scada::NodeId& device_id) override;

 private:
  Delegate* delegate_ = nullptr;

  std::shared_ptr<scada::MonitoredItem> monitored_item_;
};
