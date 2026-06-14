#pragma once

#include "controller/window_info.h"

class UiCommandRegistry;

extern const WindowInfo kTableWindowInfo;

struct TableModuleContext {
  UiCommandRegistry& ui_command_registry_;
};

class TableModule : private TableModuleContext {
 public:
  explicit TableModule(TableModuleContext&& context);
};

void RegisterTableCommandActions(UiCommandRegistry& ui_command_registry);
