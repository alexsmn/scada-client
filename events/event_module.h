#pragma once

#include "scada/services.h"

#include <memory>

class ControllerRegistry;
class Executor;
class EventFetcher;
class LocalEvents;
class Logger;
class NodeEventProvider;
class Profile;

struct EventModuleContext {
  std::shared_ptr<Executor> executor_;
  std::shared_ptr<const Logger> logger_;
  Profile& profile_;
  scada::services services_;
  ControllerRegistry& controller_registry_;
};

class EventModule : private EventModuleContext {
 public:
  explicit EventModule(EventModuleContext&& context);
  ~EventModule();

  NodeEventProvider& node_event_provider();
  EventFetcher& event_fetcher() { return *event_fetcher_; }
  LocalEvents& local_events() { return *local_events_; }

 private:
  std::shared_ptr<EventFetcher> event_fetcher_;
  std::unique_ptr<LocalEvents> local_events_;
};
