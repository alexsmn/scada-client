#pragma once

class UiCommandRegistry;
class OpenedViewCommandRegistry;

struct ExcelExportModuleContext {
  UiCommandRegistry& ui_command_registry_;
  OpenedViewCommandRegistry& opened_view_commands_;
};

class ExcelExportModule : private ExcelExportModuleContext {
 public:
  explicit ExcelExportModule(ExcelExportModuleContext&& context);
};
