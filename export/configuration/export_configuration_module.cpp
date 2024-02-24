#include "export/configuration/export_configuration_module.h"

#include "common_resources.h"
#include "controller/command_registry.h"
#include "core/main_command_context.h"
#include "export/configuration/diff_report.h"
#include "export/configuration/excel_configuration_commands.h"

ExportConfigurationModule::ExportConfigurationModule(
    ExportConfigurationModuleContext&& context)
    : ExportConfigurationModuleContext{std::move(context)} {
  // Export command.

  auto export_command =
      std::make_shared<ExportConfigurationCommand>(node_service_);

  main_commands_.AddCommand(
      BasicCommand<MainCommandContext>{ID_EXPORT_CONFIGURATION_TO_EXCEL}
          .set_execute_handler(
              [export_command](const MainCommandContext& context) {
                export_command->Execute(context.dialog_service);
              }));

  // Import command.

  auto import_command = std::make_shared<ImportConfigurationCommand>(
      node_service_, task_manager_);

  main_commands_.AddCommand(
      BasicCommand<MainCommandContext>{ID_IMPORT_CONFIGURATION_FROM_EXCEL}
          .set_execute_handler(
              [import_command](const MainCommandContext& context) {
                import_command->Execute(context.dialog_service);
              }));
}
