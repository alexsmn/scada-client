#pragma once

#include "controller/window_info.h"

class UiCommandRegistry;

extern const WindowInfo kGraphWindowInfo;

struct GraphModuleContext {
  UiCommandRegistry& ui_command_registry_;
};

class GraphModule : private GraphModuleContext {
 public:
  explicit GraphModule(GraphModuleContext&& context);
};

void RegisterGraphCommandActions(UiCommandRegistry& ui_command_registry);
