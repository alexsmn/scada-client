#pragma once

#include "components/watch/watch_event_source.h"

#include <memory>
#include <vector>

class WatchCombinedEventSource : public WatchEventSource {
 public:
  using EventSources = std::vector<std::shared_ptr<WatchEventSource>>;

  explicit WatchCombinedEventSource(EventSources event_sources)
      : event_sources_{std::move(event_sources)} {}

  // WatchEventSource

  virtual void Start(const scada::NodeId& device_id,
                     const scada::DateTimeRange& time_range,
                     Delegate& delegate) override {
    for (auto& event_source : event_sources_) {
      event_source->Start(device_id, time_range, delegate);
    }
  }

 private:
  EventSources event_sources_;
};
