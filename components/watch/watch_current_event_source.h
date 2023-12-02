#pragma once

#include "components/watch/watch_event_source.h"
#include "scada/client_monitored_item.h"

#include <memory>

class Executor;
class NodeService;

struct WatchCurrentEventSourceContext {
  std::shared_ptr<Executor> executor_;
  NodeService& node_service_;
};

class WatchCurrentEventSource final : private WatchCurrentEventSourceContext,
                                      public WatchEventSource {
 public:
  explicit WatchCurrentEventSource(WatchCurrentEventSourceContext&& context);

  // WatchEventSource
  virtual void Start(const scada::NodeId& device_id,
                     const scada::DateTimeRange& time_range,
                     Delegate& delegate) override;

 private:
  scada::monitored_item monitored_item_;
};
