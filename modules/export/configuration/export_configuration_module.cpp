#include "export/configuration/export_configuration_module.h"

#include "resources/common_resources.h"
#include "aui/translation.h"
#include "base/awaitable.h"
#include "controller/command_registry.h"
#include "controller/command_ui_registry.h"
#include "core/global_command_context.h"
#include "export/configuration/diff_report.h"
#include "export/configuration/excel_configuration_commands.h"

ExportConfigurationModule::ExportConfigurationModule(
    ExportConfigurationModuleContext&& context)
    : ExportConfigurationModuleContext{std::move(context)} {
  // Export command.

  auto export_command =
      std::make_shared<ExportConfigurationCommand>(node_service_, executor_);

  global_commands_.AddCommand(
      BasicCommand<GlobalCommandContext>{ID_EXPORT_CONFIGURATION_TO_EXCEL}
          .set_execute_handler(
              [export_command](const GlobalCommandContext& context) {
                CoSpawn(export_command->executor_,
                        [export_command,
                         &dialog_service = context.dialog_service]()
                        -> Awaitable<void> {
                          co_await export_command->Execute(dialog_service);
                          co_return;
                        });
              }));
  ui_command_registry_.AddMenuItem(
      {.menu_id = MainMenuId::More,
       .order = 300,
       .command_id = ID_EXPORT_CONFIGURATION_TO_EXCEL,
       .title = Translate("Export Configuration to Excel..."),
       .separator_before = true,
       .admin_only = true});

  // Import command.

  auto import_command = std::make_shared<ImportConfigurationCommand>(
      node_service_, task_manager_, executor_);

  global_commands_.AddCommand(
      BasicCommand<GlobalCommandContext>{ID_IMPORT_CONFIGURATION_FROM_EXCEL}
          .set_execute_handler(
              [import_command](const GlobalCommandContext& context) {
                CoSpawn(import_command->executor_,
                        [import_command,
                         &dialog_service = context.dialog_service]()
                        -> Awaitable<void> {
                          co_await import_command->Execute(dialog_service);
                          co_return;
                        });
              }));
  ui_command_registry_.AddMenuItem(
      {.menu_id = MainMenuId::More,
       .order = 310,
       .command_id = ID_IMPORT_CONFIGURATION_FROM_EXCEL,
       .title = Translate("Import Configuration from Excel..."),
       .admin_only = true});
}
