#pragma once

#include <memory>

class PrintService;

struct PrintModuleContext {
};

class PrintModule : private PrintModuleContext {
 public:
  explicit PrintModule(PrintModuleContext&& context = {});
  ~PrintModule();

  PrintService& print_service() { return *print_service_; }

 private:
  std::unique_ptr<PrintService> print_service_;
};
