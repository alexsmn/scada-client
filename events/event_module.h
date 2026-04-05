#pragma once

#include "scada/services.h"

#include <memory>
#include <string_view>

template <class T>
class BasicCommandRegistry;

class ControllerRegistry;
class Executor;
class EventFetcher;
class LocalEvents;
class Logger;
class NodeEventProvider;
class Profile;
struct SelectionCommandContext;
struct WindowInfo;

struct EventModuleContext {
  std::shared_ptr<Executor> executor_;
  std::shared_ptr<const Logger> logger_;
  Profile& profile_;
  scada::services services_;
  ControllerRegistry& controller_registry_;
  BasicCommandRegistry<SelectionCommandContext>& selection_commands_;
};

class EventModule : private EventModuleContext {
 public:
  explicit EventModule(EventModuleContext&& context);
  ~EventModule();

  NodeEventProvider& node_event_provider();
  EventFetcher& event_fetcher() { return *event_fetcher_; }
  LocalEvents& local_events() { return *local_events_; }

 private:
  void AddOpenCommand(unsigned command_id,
                      const WindowInfo& window_info,
                      const std::string_view& mode = {});

  std::shared_ptr<EventFetcher> event_fetcher_;
  std::unique_ptr<LocalEvents> local_events_;
};
