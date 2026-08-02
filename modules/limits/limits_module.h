#pragma once

#include "base/any_executor.h"

template <class T>
class BasicCommandRegistry;

class TaskManager;
class UiCommandRegistry;
struct SelectionCommandContext;

namespace scada {
class SessionService;
}

struct LimitsModuleContext {
  AnyExecutor executor_;
  scada::SessionService& session_service_;
  TaskManager& task_manager_;
  BasicCommandRegistry<SelectionCommandContext>& selection_commands_;
  UiCommandRegistry& ui_command_registry_;
};

class LimitsModule : private LimitsModuleContext {
 public:
  explicit LimitsModule(LimitsModuleContext&& context);
};
