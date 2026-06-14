#include "app/client_application_modules.h"

#include "configuration/configuration_module.h"
#include "export/configuration/export_configuration_module.h"
#include "filesystem/filesystem_component.h"
#include "modules/debugger/debugger_module.h"
#if defined(UI_QT)
#include "modules/graph/graph_component.h"
#endif
#include "modules/node_service_progress_tracker/node_service_progress_tracker.h"
#include "modules/opcua_services/opcua_services_module.h"
#include "modules/sheet/sheet_component.h"
#include "modules/summary/summary_component.h"
#include "modules/table/table_component.h"
#include "modules/timed_data/timed_data_component.h"
#include "modules/watch/watch_component.h"
#include "print/service/print_module.h"

#if CLIENT_HAS_MODUS
#include "modus/modus_module.h"
#endif
#if CLIENT_HAS_VIDICON
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
#if defined(UI_QT)
    context.singletons_.emplace(
        std::make_shared<GraphModule>(GraphModuleContext{
            .ui_command_registry_ = context.ui_command_registry_}));
#endif
    context.singletons_.emplace(
        std::make_shared<TableModule>(TableModuleContext{
            .ui_command_registry_ = context.ui_command_registry_}));
    context.singletons_.emplace(
        std::make_shared<SheetModule>(SheetModuleContext{
            .ui_command_registry_ = context.ui_command_registry_}));
    context.singletons_.emplace(
        std::make_shared<SummaryModule>(SummaryModuleContext{
            .ui_command_registry_ = context.ui_command_registry_}));
    context.singletons_.emplace(
        std::make_shared<TimedDataModule>(TimedDataModuleContext{
            .ui_command_registry_ = context.ui_command_registry_}));
    context.singletons_.emplace(
        std::make_shared<WatchModule>(WatchModuleContext{
            .ui_command_registry_ = context.ui_command_registry_}));

    if (modules.configuration) {
      context.singletons_.emplace(
          std::make_shared<ConfigurationModule>(ConfigurationModuleContext{
              .executor_ = context.executor_,
              .controller_registry_ = context.controller_registry_,
              .profile_ = context.profile_,
              .node_service_tree_factory_ = context.node_service_tree_factory_,
              .session_service_ = *context.scada_services_.session_service,
              .local_events_ = context.local_events_,
              .task_manager_ = context.task_manager_,
              .selection_commands_ = context.selection_commands_,
              .ui_command_registry_ = context.ui_command_registry_}));
    }

    if (modules.debugger) {
      context.singletons_.emplace(
          std::make_shared<DebuggerModule>(DebuggerModuleContext{
              .session_service_ = *context.scada_services_.session_service,
              .global_commands_ = context.global_commands_,
              .selection_commands_ = context.selection_commands_,
              .ui_command_registry_ = context.ui_command_registry_}));
    }

#if CLIENT_HAS_MODUS
    if (modules.modus) {
      context.singletons_.emplace(
          std::make_shared<ModusModule>(ModusModuleContext{
              .controller_registry_ = context.controller_registry_,
              .blinker_manager_ = context.blinker_manager_,
              .file_registry_ = context.filesystem_component_.file_registry(),
              .global_commands_ = context.global_commands_,
              .ui_command_registry_ = context.ui_command_registry_,
              .profile_ = context.profile_,
              .alias_resolver_ = context.alias_resolver_}));
    }
#endif

#if CLIENT_HAS_VIDICON
    if (modules.vidicon) {
      context.singletons_.emplace(
          std::make_shared<VidiconModule>(VidiconModuleContext{
              .executor_ = context.executor_,
              .timed_data_service_ = context.timed_data_service_,
              .controller_registry_ = context.controller_registry_,
              .write_service_ = context.write_service_,
              .file_registry_ =
                  context.filesystem_component_.file_registry()}));
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
