#pragma once

#include "base/any_executor.h"

#include "base/cancelation.h"
#include "base/time_range.h"
#include "modules/watch/watch_event_source.h"
#include "scada/node_id.h"

#include <memory>

class NodeService;

struct WatchHistorySourceContext {
  AnyExecutor executor_;
  NodeService& node_service_;
};

class WatchHistoryEventSource : private WatchHistorySourceContext,
                                public WatchEventSource {
 public:
  explicit WatchHistoryEventSource(WatchHistorySourceContext&& context);

  // WatchEventSource
  virtual void Start(const scada::NodeId& device_id,
                     const scada::DateTimeRange& time_range,
                     Delegate& delegate) override;

 private:
  Cancelation cancelation_;
};
