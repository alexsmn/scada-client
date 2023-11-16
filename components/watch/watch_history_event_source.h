#pragma once

#include "components/watch/watch_event_source.h"
#include "scada/client_monitored_item.h"

#include <memory>

class NodeService;

struct WatchHistorySourceContext {
  NodeService& node_service_;
};

class WatchHistoryEventSource : private WatchHistorySourceContext,
                                public WatchEventSource {
 public:
  explicit WatchHistoryEventSource(WatchHistorySourceContext&& context);

  // WatchEventSource
  virtual void SetDelegate(Delegate* delegate) override;
  virtual void SetDeviceId(const scada::NodeId& device_id) override;

 private:
  Delegate* delegate_ = nullptr;
};
