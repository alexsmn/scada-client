#include "app/client_application_modules.h"

#include "modules/debugger/debugger_module.h"
#include "modules/node_service_progress_tracker/node_service_progress_tracker.h"
#include "modules/opcua_services/opcua_services_module.h"
#include "configuration/configuration_module.h"
#include "export/configuration/export_configuration_module.h"
#include "filesystem/filesystem_component.h"
#include "print/service/print_module.h"

#if !defined(UI_WT)
#include "modus/modus_module.h"
#include "vidicon/vidicon_module.h"
#endif

void RegisterClientApplicationModules(ClientApplicationModules modules) {
  if (modules.opcua_services) {
    static auto opcua_services_module =
        std::make_shared<OpcUaServicesModule>(OpcUaServicesModuleContext{});
    (void)opcua_services_module;
  }
}

ClientApplicationModuleConfigurator MakeDefaultClientApplicationModules(
    ClientApplicationModules modules) {
  return [modules](ClientApplicationModuleContext& context) {
    if (modules.configuration) {
      context.singletons_.emplace(std::make_shared<ConfigurationModule>(
          ConfigurationModuleContext{
              .controller_registry_ = context.controller_registry_,
              .profile_ = context.profile_,
              .node_service_tree_factory_ = context.node_service_tree_factory_,
              .ui_command_registry_ = context.ui_command_registry_}));
    }

    if (modules.debugger) {
      context.singletons_.emplace(std::make_shared<DebuggerModule>(
          DebuggerModuleContext{
              .session_service_ = *context.scada_services_.session_service,
              .global_commands_ = context.global_commands_,
              .selection_commands_ = context.selection_commands_,
              .ui_command_registry_ = context.ui_command_registry_}));
    }

#if !defined(UI_WT)
    if (modules.modus) {
      context.singletons_.emplace(std::make_shared<ModusModule>(
          ModusModuleContext{
              .controller_registry_ = context.controller_registry_,
              .blinker_manager_ = context.blinker_manager_,
              .file_registry_ = context.filesystem_component_.file_registry(),
              .global_commands_ = context.global_commands_,
              .ui_command_registry_ = context.ui_command_registry_,
              .profile_ = context.profile_,
              .alias_resolver_ = context.alias_resolver_}));
    }

    if (modules.vidicon) {
      context.singletons_.emplace(std::make_shared<VidiconModule>(
          VidiconModuleContext{
              .executor_ = context.executor_,
              .timed_data_service_ = context.timed_data_service_,
              .controller_registry_ = context.controller_registry_,
              .write_service_ = context.write_service_,
              .file_registry_ = context.filesystem_component_.file_registry()}));
    }
#endif

    if (modules.opcua_services) {
      context.singletons_.emplace(
          std::make_shared<OpcUaServicesModule>(OpcUaServicesModuleContext{}));
    }

    if (modules.print) {
      context.print_module_ =
          std::make_unique<PrintModule>(PrintModuleContext{});
    }

    if (modules.export_configuration) {
      context.singletons_.emplace(std::make_shared<ExportConfigurationModule>(
          ExportConfigurationModuleContext{
              .executor_ = context.executor_,
              .node_service_ = context.node_service_,
              .task_manager_ = context.task_manager_,
              .global_commands_ = context.global_commands_,
              .ui_command_registry_ = context.ui_command_registry_}));
    }

    if (modules.node_service_progress_tracker) {
      context.singletons_.emplace(std::make_shared<NodeServiceProgressTracker>(
          context.executor_, context.node_service_, context.progress_host_));
    }
  };
}
