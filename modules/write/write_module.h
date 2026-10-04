#pragma once

#include "base/any_executor.h"

template <class T>
class BasicCommandRegistry;

class LocalEvents;
class Profile;
class TimedDataService;
class UiCommandRegistry;
struct SelectionCommandContext;

namespace scada {
class SessionService;
}

struct WriteModuleContext {
  AnyExecutor executor_;
  TimedDataService& timed_data_service_;
  scada::SessionService& session_service_;
  Profile& profile_;
  // Where a successful control command is reported (backlog 849).
  LocalEvents& local_events_;
  BasicCommandRegistry<SelectionCommandContext>& selection_commands_;
  UiCommandRegistry& ui_command_registry_;
};

class WriteModule : private WriteModuleContext {
 public:
  explicit WriteModule(WriteModuleContext&& context);
};
