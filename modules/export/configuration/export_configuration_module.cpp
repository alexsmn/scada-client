#include "export/configuration/export_configuration_module.h"

#include "resources/common_resources.h"
#include "base/awaitable.h"
#include "controller/action_manager.h"
#include "core/global_command_context.h"
#include "export/configuration/diff_report.h"
#include "export/configuration/excel_configuration_commands.h"

ExportConfigurationModule::ExportConfigurationModule(
    ExportConfigurationModuleContext&& context)
    : ExportConfigurationModuleContext{std::move(context)} {
  // Export command.

  auto export_command =
      std::make_shared<ExportConfigurationCommand>(node_service_, executor_);

  action_manager_.AddAction(
      Action{.command_id_ = ID_EXPORT_CONFIGURATION_TO_EXCEL,
             .execute_handler_ = MakeContextHandler<GlobalCommandContext>(
              [export_command](const GlobalCommandContext& context) {
                CoSpawn(export_command->executor_,
                        [export_command,
                         &dialog_service = context.dialog_service]()
                        -> Awaitable<void> {
                          co_await export_command->Execute(dialog_service);
                          co_return;
                        });
              })});

  // Import command.

  auto import_command = std::make_shared<ImportConfigurationCommand>(
      node_service_, task_manager_, executor_);

  action_manager_.AddAction(
      Action{.command_id_ = ID_IMPORT_CONFIGURATION_FROM_EXCEL,
             .execute_handler_ = MakeContextHandler<GlobalCommandContext>(
              [import_command](const GlobalCommandContext& context) {
                CoSpawn(import_command->executor_,
                        [import_command,
                         &dialog_service = context.dialog_service]()
                        -> Awaitable<void> {
                          co_await import_command->Execute(dialog_service);
                          co_return;
                        });
              })});
}
