#pragma once

#include "controller/window_info.h"

class UiCommandRegistry;

extern const WindowInfo kWatchWindowInfo;

struct WatchModuleContext {
  UiCommandRegistry& ui_command_registry_;
};

class WatchModule : private WatchModuleContext {
 public:
  explicit WatchModule(WatchModuleContext&& context);
};

void RegisterWatchCommandActions(UiCommandRegistry& ui_command_registry);
