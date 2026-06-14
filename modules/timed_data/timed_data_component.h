#pragma once

#include "controller/window_info.h"

class UiCommandRegistry;

extern const WindowInfo kTimedDataWindowInfo;

struct TimedDataModuleContext {
  UiCommandRegistry& ui_command_registry_;
};

class TimedDataModule : private TimedDataModuleContext {
 public:
  explicit TimedDataModule(TimedDataModuleContext&& context);
};

void RegisterTimedDataCommandActions(UiCommandRegistry& ui_command_registry);
