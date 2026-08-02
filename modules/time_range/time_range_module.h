#pragma once

class UiCommandRegistry;
class OpenedViewCommandRegistry;

struct TimeRangeModuleContext {
  UiCommandRegistry& ui_command_registry_;
  OpenedViewCommandRegistry& opened_view_commands_;
};

class TimeRangeModule : private TimeRangeModuleContext {
 public:
  explicit TimeRangeModule(TimeRangeModuleContext&& context);
};
