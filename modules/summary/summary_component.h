#pragma once

#include "controller/window_info.h"

class UiCommandRegistry;

extern const WindowInfo kSummaryWindowInfo;

struct SummaryModuleContext {
  UiCommandRegistry& ui_command_registry_;
};

class SummaryModule : private SummaryModuleContext {
 public:
  explicit SummaryModule(SummaryModuleContext&& context);
};

void RegisterSummaryCommandActions(UiCommandRegistry& ui_command_registry);
