#pragma once

#include "scada/date_time_range.h"

namespace scada {
class Event;
class NodeId;
class Status;
}  // namespace scada

struct TimeRange;

class WatchEventSource {
 public:
  class Delegate {
   public:
    virtual ~Delegate() = default;

    virtual void OnEvent(const scada::Event& event) = 0;
    virtual void OnError(const scada::Status& status) = 0;
  };

  virtual ~WatchEventSource() = default;

  // If `time_range.first == max`, then show only current events.
  // If `time_range.second != max`, then show only historical events.
  virtual void Start(const scada::NodeId& device_id,
                     const scada::DateTimeRange& time_range,
                     Delegate& delegate) = 0;
};
