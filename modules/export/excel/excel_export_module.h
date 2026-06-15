#pragma once

class UiCommandRegistry;

struct ExcelExportModuleContext {
  UiCommandRegistry& ui_command_registry_;
};

class ExcelExportModule : private ExcelExportModuleContext {
 public:
  explicit ExcelExportModule(ExcelExportModuleContext&& context);
};
