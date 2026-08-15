#include "screenshot_modules.h"

#include "configuration/configuration_module.h"
#include "export/configuration/export_configuration_module.h"
#include "modules/node_service_progress_tracker/node_service_progress_tracker.h"
#include "modules/summary/summary_component.h"

ClientApplicationModuleConfigurator MakeScreenshotModules() {
  return [](ClientApplicationModuleContext& context) {
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

    // The two modules below register no view of their own — the views come
    // from REGISTER_CONTROLLER, which is why captures of them worked without
    // these. What they register is *menu* chrome, which has no such static
    // fallback: a module that is not installed contributes no commands, so the
    // menus that would list them render empty. The menu captures are the only
    // thing that reads those menus, so this is where the gap surfaced.

    // Contributes "Export/Import Configuration to Excel..." to the More menu
    // (menu-excel.png). Both rows are `admin_only`, so they additionally need
    // the fixture's session service to grant kConfigure.
    context.singletons_.emplace(std::make_shared<ExportConfigurationModule>(
        ExportConfigurationModuleContext{
            .executor_ = context.executor_,
            .node_service_ = context.node_service_,
            .task_manager_ = context.task_manager_,
            .global_commands_ = context.global_commands_,
            .ui_command_registry_ = context.ui_command_registry_}));

    // Registers the seven aggregate-function actions the command toolbar
    // groups behind its "Function" button (menu-summary.png). Without it the
    // Summary view still opens and still answers those command ids — its own
    // registry holds the handlers — but nothing declares them to the toolbar,
    // so the button does not exist.
    context.singletons_.emplace(
        std::make_shared<SummaryModule>(SummaryModuleContext{
            .executor_ = context.executor_,
            .selection_commands_ = context.selection_commands_,
            .ui_command_registry_ = context.ui_command_registry_}));

    context.singletons_.emplace(std::make_shared<NodeServiceProgressTracker>(
        context.executor_, context.node_service_, context.progress_host_));
  };
}
