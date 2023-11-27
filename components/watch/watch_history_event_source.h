#pragma once

#include "base/cancelation.h"
#include "base/time_range.h"
#include "components/watch/watch_event_source.h"
#include "scada/node_id.h"

#include <memory>

class Executor;
class NodeService;

struct WatchHistorySourceContext {
  std::shared_ptr<Executor> executor_;
  NodeService& node_service_;
};

class WatchHistoryEventSource : private WatchHistorySourceContext,
                                public WatchEventSource {
 public:
  explicit WatchHistoryEventSource(WatchHistorySourceContext&& context);

  // WatchEventSource
  virtual void SetDelegate(Delegate* delegate) override;
  virtual void SetDeviceId(const scada::NodeId& device_id) override;
  virtual void SetTimeRange(const TimeRange& time_range) override;

 private:
  void FetchEvents();

  Delegate* delegate_ = nullptr;

  scada::NodeId device_id_;
  TimeRange time_range_ = base::TimeDelta::FromMinutes(15);

  Cancelation cancelation_;
};
