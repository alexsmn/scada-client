#include "print/service/print_module.h"

#include "aui/translation.h"
#include "controller/action.h"
#include "controller/command_ui_registry.h"
#include "main_window/opened_view/opened_view_command_registry.h"
#include "print/service/print_command.h"
#include "print/service/print_service.h"
#include "resources/common_resources.h"

PrintModule::PrintModule(PrintModuleContext&& context)
    : PrintModuleContext{std::move(context)},
      print_service_{std::make_unique<PrintService>()} {
  ui_command_registry_.AddAction(Action{.command_id_ = ID_PRINT,
                                        .category_ = CATEGORY_SETUP,
                                        .title_ = Translate("Print"),
                                        .image_id_ = IDB_PRINTER});
  opened_view_commands_.AddFactory(
      [](const OpenedViewCommandFactoryContext& context)
          -> std::unique_ptr<CommandHandler> {
        if (!context.print_service_) {
          return std::unique_ptr<CommandHandler>();
        }
        return std::make_unique<PrintCommand>(PrintCommandContext{
            .print_service_ = *context.print_service_,
            .dialog_service_ = context.dialog_service_,
            .export_model_getter_ = context.export_model_getter_,
            .print_view_handler_ = context.print_view_handler_});
      });
}

PrintModule::~PrintModule() = default;
