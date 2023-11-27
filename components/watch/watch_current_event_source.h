#pragma once

#include "components/watch/watch_event_source.h"
#include "scada/client_monitored_item.h"

#include <memory>

class NodeService;

struct WatchCurrentEventSourceContext {
  NodeService& node_service_;
};

class WatchCurrentEventSource final : private WatchCurrentEventSourceContext,
                                      public WatchEventSource {
 public:
  explicit WatchCurrentEventSource(WatchCurrentEventSourceContext&& context);

  // WatchEventSource
  virtual void SetDelegate(Delegate* delegate) override;
  virtual void SetDeviceId(const scada::NodeId& device_id) override;
  virtual void SetTimeRange(const TimeRange& time_range) override;

 private:
  Delegate* delegate_ = nullptr;

  scada::monitored_item monitored_item_;
};
