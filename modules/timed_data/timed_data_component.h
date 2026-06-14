#pragma once

#include "base/any_executor.h"
#include "controller/window_info.h"

template <class T>
class BasicCommandRegistry;

struct SelectionCommandContext;
class UiCommandRegistry;

extern const WindowInfo kTimedDataWindowInfo;

struct TimedDataModuleContext {
  AnyExecutor executor_;
  BasicCommandRegistry<SelectionCommandContext>& selection_commands_;
  UiCommandRegistry& ui_command_registry_;
};

class TimedDataModule : private TimedDataModuleContext {
 public:
  explicit TimedDataModule(TimedDataModuleContext&& context);
};

void RegisterTimedDataCommandActions(UiCommandRegistry& ui_command_registry);
