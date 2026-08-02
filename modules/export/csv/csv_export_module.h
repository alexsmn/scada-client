#pragma once

#include <memory>

class UiCommandRegistry;
class OpenedViewCommandRegistry;

struct CsvExportModuleContext {
  UiCommandRegistry& ui_command_registry_;
  OpenedViewCommandRegistry& opened_view_commands_;
};

class CsvExportModule : private CsvExportModuleContext {
 public:
  explicit CsvExportModule(CsvExportModuleContext&& context);
  ~CsvExportModule();
};
