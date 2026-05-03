#include "print/service/print_module.h"

#include "print/service/print_service.h"

PrintModule::PrintModule(PrintModuleContext&& context)
    : PrintModuleContext{std::move(context)},
      print_service_{std::make_unique<PrintService>()} {
}

PrintModule::~PrintModule() = default;
