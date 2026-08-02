#pragma once

#include "scada/date_time_range.h"
#include "scada/event.h"

namespace scada {
class NodeId;
class Status;
struct Event;
}  // namespace scada

namespace scada { struct RelativeTimeRange; }

class WatchEventSource {
 public:
  class Delegate {
   public:
    virtual ~Delegate() = default;

    virtual void OnEvent(const scada::Event& event) = 0;
    // A device protocol frame. Defaulted to the base event so a delegate that
    // does not care about frames still receives the log line.
    virtual void OnDeviceFrame(const scada::DeviceFrameEvent& event) {
      OnEvent(event.base);
    }
    virtual void OnError(const scada::Status& status) = 0;
  };

  virtual ~WatchEventSource() = default;

  // If `time_range.first == max`, then show only current events.
  // If `time_range.second != max`, then show only historical events.
  virtual void Start(const scada::NodeId& device_id,
                     const scada::TimeRange& time_range,
                     Delegate& delegate) = 0;
};
