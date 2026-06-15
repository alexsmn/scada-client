#include "print/service/print_module.h"

#include "aui/translation.h"
#include "controller/action.h"
#include "controller/command_ui_registry.h"
#include "print/service/print_service.h"
#include "resources/common_resources.h"

PrintModule::PrintModule(PrintModuleContext&& context)
    : PrintModuleContext{std::move(context)},
      print_service_{std::make_unique<PrintService>()} {
  ui_command_registry_.AddAction(Action{.command_id_ = ID_PRINT,
                                        .category_ = CATEGORY_SETUP,
                                        .title_ = Translate("Print"),
                                        .image_id_ = IDB_PRINTER});
}

PrintModule::~PrintModule() = default;
