#pragma once

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

  virtual void SetDelegate(Delegate* delegate) = 0;

  virtual void SetDeviceId(const scada::NodeId& device_id) = 0;

  virtual void SetTimeRange(const TimeRange& time_range) = 0;
};
