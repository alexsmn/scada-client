#pragma once

#include <memory>

struct CsvExportModuleContext {
};

class CsvExportModule : private CsvExportModuleContext {
 public:
  explicit CsvExportModule(CsvExportModuleContext&& context);
  ~CsvExportModule();
};
