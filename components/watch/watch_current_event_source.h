#pragma once

#include "components/watch/watch_event_source.h"
#include "scada/client_monitored_item.h"

#include <memory>

class NodeService;

struct WatchEventSourceContext {
  NodeService& node_service_;
};

class WatchCurrentEventSource : private WatchEventSourceContext,
                                public WatchEventSource {
 public:
  explicit WatchCurrentEventSource(WatchEventSourceContext&& context);

  // WatchEventSource
  virtual void SetDelegate(Delegate* delegate) override;
  virtual void SetDeviceId(const scada::NodeId& device_id) override;

 private:
  Delegate* delegate_ = nullptr;

  scada::monitored_item monitored_item_;
};
