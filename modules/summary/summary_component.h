#pragma once

#include "base/any_executor.h"
#include "controller/window_info.h"

template <class T>
class BasicCommandRegistry;

struct SelectionCommandContext;
class UiCommandRegistry;

extern const WindowInfo kSummaryWindowInfo;

struct SummaryModuleContext {
  AnyExecutor executor_;
  BasicCommandRegistry<SelectionCommandContext>& selection_commands_;
  UiCommandRegistry& ui_command_registry_;
};

class SummaryModule : private SummaryModuleContext {
 public:
  explicit SummaryModule(SummaryModuleContext&& context);
};

void RegisterSummaryCommandActions(UiCommandRegistry& ui_command_registry);
