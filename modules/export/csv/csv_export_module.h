#pragma once

#include <memory>

class UiCommandRegistry;

struct CsvExportModuleContext {
  UiCommandRegistry& ui_command_registry_;
};

class CsvExportModule : private CsvExportModuleContext {
 public:
  explicit CsvExportModule(CsvExportModuleContext&& context);
  ~CsvExportModule();
};
