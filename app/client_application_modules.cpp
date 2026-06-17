#include "app/client_application_modules.h"

#include "configuration/configuration_module.h"
#include "export/configuration/export_configuration_module.h"
#include "filesystem/filesystem_component.h"
#include "modules/about/about_dialog.h"
#include "modules/create/create_module.h"
#include "modules/debugger/debugger_module.h"
#include "modules/device_metrics/device_metrics_command.h"
#if defined(_WIN32)
#include "modules/web/web_component.h"
#endif
#if defined(UI_QT)
#include "modules/graph/graph_component.h"
#endif
#include "modules/change_password/change_password_module.h"
#include "modules/node_properties/node_property_component.h"
#include "modules/node_service_progress_tracker/node_service_progress_tracker.h"
#include "modules/opcua_services/opcua_services_module.h"
#include "modules/selection_edit/selection_edit_module.h"
#include "modules/sheet/sheet_component.h"
#include "modules/summary/summary_component.h"
#include "modules/table/table_component.h"
#include "modules/time_range/time_range_module.h"
#include "modules/timed_data/timed_data_component.h"
#include "modules/transmission/transmission_component.h"
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
    RegisterAboutCommands(context.global_commands_);
#if defined(_WIN32)
    RegisterWebCommands(context.executor_, context.global_commands_);
#endif

#if defined(UI_QT)
    context.singletons_.emplace(
        std::make_shared<GraphModule>(GraphModuleContext{
            .executor_ = context.executor_,
            .file_cache_ = context.filesystem_component_.file_cache(),
            .global_commands_ = context.global_commands_,
            .selection_commands_ = context.selection_commands_,
            .ui_command_registry_ = context.ui_command_registry_}));
#endif
    context.singletons_.emplace(
        std::make_shared<TableModule>(TableModuleContext{
            .executor_ = context.executor_,
            .session_service_ = *context.scada_services_.session_service,
            .global_commands_ = context.global_commands_,
            .selection_commands_ = context.selection_commands_,
            .ui_command_registry_ = context.ui_command_registry_}));
    context.singletons_.emplace(
        std::make_shared<SheetModule>(SheetModuleContext{
            .ui_command_registry_ = context.ui_command_registry_}));
    context.singletons_.emplace(
        std::make_shared<SummaryModule>(SummaryModuleContext{
            .executor_ = context.executor_,
            .selection_commands_ = context.selection_commands_,
            .ui_command_registry_ = context.ui_command_registry_}));
    context.singletons_.emplace(
        std::make_shared<TimeRangeModule>(TimeRangeModuleContext{
            .ui_command_registry_ = context.ui_command_registry_,
            .opened_view_commands_ = context.opened_view_commands_}));
    context.singletons_.emplace(
        std::make_shared<TimedDataModule>(TimedDataModuleContext{
            .executor_ = context.executor_,
            .selection_commands_ = context.selection_commands_,
            .ui_command_registry_ = context.ui_command_registry_}));
    context.singletons_.emplace(
        std::make_shared<WatchModule>(WatchModuleContext{
            .executor_ = context.executor_,
            .selection_commands_ = context.selection_commands_,
            .ui_command_registry_ = context.ui_command_registry_}));
    context.singletons_.emplace(
        std::make_shared<TransmissionModule>(TransmissionModuleContext{
            .executor_ = context.executor_,
            .session_service_ = *context.scada_services_.session_service,
            .selection_commands_ = context.selection_commands_,
            .ui_command_registry_ = context.ui_command_registry_}));
    context.singletons_.emplace(
        std::make_shared<NodePropertyModule>(NodePropertyModuleContext{
            .executor_ = context.executor_,
            .session_service_ = *context.scada_services_.session_service,
            .selection_commands_ = context.selection_commands_,
            .ui_command_registry_ = context.ui_command_registry_}));
    context.singletons_.emplace(
        std::make_shared<DeviceMetricsModule>(DeviceMetricsModuleContext{
            .executor_ = context.executor_,
            .selection_commands_ = context.selection_commands_,
            .ui_command_registry_ = context.ui_command_registry_}));
    context.singletons_.emplace(
        std::make_shared<SelectionEditModule>(SelectionEditModuleContext{
            .executor_ = context.executor_,
            .session_service_ = *context.scada_services_.session_service,
            .node_service_ = context.node_service_,
            .task_manager_ = context.task_manager_,
            .selection_commands_ = context.selection_commands_,
            .ui_command_registry_ = context.ui_command_registry_,
            .opened_view_commands_ = context.opened_view_commands_}));
    context.singletons_.emplace(
        std::make_shared<ChangePasswordModule>(ChangePasswordModuleContext{
            .executor_ = context.executor_,
            .local_events_ = context.local_events_,
            .profile_ = context.profile_,
            .session_service_ = *context.scada_services_.session_service,
            .selection_commands_ = context.selection_commands_,
            .ui_command_registry_ = context.ui_command_registry_}));
    context.singletons_.emplace(
        std::make_shared<CreateModule>(CreateModuleContext{
            .node_service_ = context.node_service_,
            .ui_command_registry_ = context.ui_command_registry_,
            .opened_view_commands_ = context.opened_view_commands_}));

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
      context.print_module_ = std::make_unique<PrintModule>(PrintModuleContext{
          .ui_command_registry_ = context.ui_command_registry_,
          .opened_view_commands_ = context.opened_view_commands_});
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
