#pragma once

#include "base/any_executor.h"
#include "controller/window_info.h"

template <class T>
class BasicCommandRegistry;

struct SelectionCommandContext;
class UiCommandRegistry;

extern const WindowInfo kWatchWindowInfo;

struct WatchModuleContext {
  AnyExecutor executor_;
  BasicCommandRegistry<SelectionCommandContext>& selection_commands_;
  UiCommandRegistry& ui_command_registry_;
};

class WatchModule : private WatchModuleContext {
 public:
  explicit WatchModule(WatchModuleContext&& context);
};

void RegisterWatchCommandActions(UiCommandRegistry& ui_command_registry);
