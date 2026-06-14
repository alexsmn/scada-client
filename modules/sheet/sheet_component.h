#pragma once

#include "controller/window_info.h"

class UiCommandRegistry;

extern const WindowInfo kSheetWindowInfo;

struct SheetModuleContext {
  UiCommandRegistry& ui_command_registry_;
};

class SheetModule : private SheetModuleContext {
 public:
  explicit SheetModule(SheetModuleContext&& context);
};

void RegisterSheetCommandActions(UiCommandRegistry& ui_command_registry);
