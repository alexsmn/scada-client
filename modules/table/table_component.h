#pragma once

#include "base/any_executor.h"
#include "controller/window_info.h"
#include "scada/session_service.h"

template <class T>
class BasicCommandRegistry;

struct GlobalCommandContext;
struct SelectionCommandContext;
class UiCommandRegistry;

extern const WindowInfo kTableWindowInfo;

struct TableModuleContext {
  AnyExecutor executor_;
  scada::SessionService& session_service_;
  BasicCommandRegistry<GlobalCommandContext>& global_commands_;
  BasicCommandRegistry<SelectionCommandContext>& selection_commands_;
  UiCommandRegistry& ui_command_registry_;
};

class TableModule : private TableModuleContext {
 public:
  explicit TableModule(TableModuleContext&& context);
};

void RegisterTableCommandActions(UiCommandRegistry& ui_command_registry);
