#include "export/configuration/export_configuration_module.h"

#include "aui/translation.h"
#include "base/awaitable.h"
#include "controller/command_registry.h"
#include "controller/command_ui_registry.h"
#include "core/global_command_context.h"
#include "export/configuration/configuration_transfer_commands.h"
#include "node_service/node_service.h"
#include "resources/common_resources.h"

ExportConfigurationModule::ExportConfigurationModule(
    ExportConfigurationModuleContext&& context)
    : ExportConfigurationModuleContext{std::move(context)} {
  ConfigurationTransferCall call = call_;
  if (!call) {
    // A plain lambda returning the awaitable, not a coroutine lambda: the
    // awaitable is lazy, and a coroutine lambda's frame would point at a
    // closure std::function copies and destroys (see the root CLAUDE.md).
    call = [&node_service = node_service_](
               scada::NodeId object_id, scada::NodeId method_id,
               std::vector<scada::Variant> arguments) {
      return node_service.GetScadaNode(object_id).call_packed_result(
          std::move(method_id), std::move(arguments));
    };
  }
  client_ =
      std::make_shared<const ConfigurationTransferClient>(std::move(call));

  global_commands_.AddCommand(
      BasicCommand<GlobalCommandContext>{ID_EXPORT_CONFIGURATION}
          .set_execute_handler([executor = executor_, client = client_](
                                   const GlobalCommandContext& context) {
            CoSpawn(executor,
                    [client, &dialog_service =
                                 context.dialog_service]() -> Awaitable<void> {
                      co_await ExportConfiguration(*client, dialog_service);
                    });
          }));
  ui_command_registry_.AddMenuItem(
      {.menu_id = MainMenuId::More,
       .order = 300,
       .command_id = ID_EXPORT_CONFIGURATION,
       .title = Translate("Export Configuration..."),
       .separator_before = true,
       .admin_only = true});

  global_commands_.AddCommand(
      BasicCommand<GlobalCommandContext>{ID_IMPORT_CONFIGURATION}
          .set_execute_handler([executor = executor_, client = client_](
                                   const GlobalCommandContext& context) {
            CoSpawn(executor,
                    [client, &dialog_service =
                                 context.dialog_service]() -> Awaitable<void> {
                      co_await ImportConfiguration(*client, dialog_service);
                    });
          }));
  ui_command_registry_.AddMenuItem(
      {.menu_id = MainMenuId::More,
       .order = 310,
       .command_id = ID_IMPORT_CONFIGURATION,
       .title = Translate("Import Configuration..."),
       .admin_only = true});
}
