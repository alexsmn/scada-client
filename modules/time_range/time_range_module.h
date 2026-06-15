#pragma once

class UiCommandRegistry;

struct TimeRangeModuleContext {
  UiCommandRegistry& ui_command_registry_;
};

class TimeRangeModule : private TimeRangeModuleContext {
 public:
  explicit TimeRangeModule(TimeRangeModuleContext&& context);
};
