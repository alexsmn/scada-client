#include "export/configuration/export_configuration_module.h"

#include "common_resources.h"
#include "controller/command_registry.h"
#include "core/main_command_context.h"
#include "export/configuration/excel_configuration_commands.h"

ExportConfigurationModule::ExportConfigurationModule(
    ExportConfigurationModuleContext&& context)
    : ExportConfigurationModuleContext{std::move(context)} {
  main_commands_.AddCommand(
      BasicCommand<MainCommandContext>{ID_EXPORT_CONFIGURATION_TO_EXCEL}
          .set_execute_handler([this](const MainCommandContext& context) {
            ExportConfigurationCommand{node_service_, context.dialog_service}
                .Execute();
          }));

  main_commands_.AddCommand(
      BasicCommand<MainCommandContext>{ID_IMPORT_CONFIGURATION_FROM_EXCEL}
          .set_execute_handler([this](const MainCommandContext& context) {
            ImportConfigurationCommand{node_service_, task_manager_,
                                       context.dialog_service}
                .Execute();
          }));
}
