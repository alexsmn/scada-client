#include "screenshot_modules.h"

#include "configuration/configuration_module.h"
#include "export/configuration/export_configuration_module.h"
#include "modules/create/create_module.h"
#include "modules/node_properties/node_property_component.h"
#include "modules/node_service_progress_tracker/node_service_progress_tracker.h"
#include "modules/selection_edit/selection_edit_module.h"
#include "modules/summary/summary_component.h"
#include "modules/table/table_component.h"
#include "modules/timed_data/timed_data_component.h"
#if defined(UI_QT)
#include "filesystem/filesystem_component.h"
#include "modules/graph/graph_component.h"
#endif

ClientApplicationModuleConfigurator MakeScreenshotModules() {
  return [](ClientApplicationModuleContext& context) {
  // The modules below register no view of their own — the views come from
  // REGISTER_CONTROLLER, which is why captures of them worked without these.
  // What they register is *menu* chrome, which has no such static fallback:
  // a module that is not installed contributes no commands, so the menus
  // that would list them render empty, or short. The menu captures are the
  // only thing that reads those menus, so this is where the gap surfaces —
  // and it surfaces as a plausible-looking image of a shorter menu rather
  // than as a failure, which is why the context captures assert on rows.
  //
  // **Install order is the shipping client's** (see
  // `MakeDefaultClientApplicationModules` in
  // `client/app/client_application_modules.cpp`). A context menu's rows come
  // out in command-registration order, so a set installed in a different
  // order renders the right rows in an order no operator sees. Keep the two
  // lists in step.

#if defined(UI_QT)
    // «График» on a data item's context menu.
    context.singletons_.emplace(
        std::make_shared<GraphModule>(GraphModuleContext{
            .executor_ = context.executor_,
            .file_cache_ = context.filesystem_component_.file_cache(),
            .global_commands_ = context.global_commands_,
            .selection_commands_ = context.selection_commands_,
            .ui_command_registry_ = context.ui_command_registry_}));
#endif
    // «Таблица», «Таблица группы» and «Свойства элемента» — the last of which
    // is what dev/displays.md's first image is about.
    context.singletons_.emplace(
        std::make_shared<TableModule>(TableModuleContext{
            .executor_ = context.executor_,
            .session_service_ = *context.scada_services_.session_service,
            .global_commands_ = context.global_commands_,
            .selection_commands_ = context.selection_commands_,
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
    // «Данные».
    context.singletons_.emplace(
        std::make_shared<TimedDataModule>(TimedDataModuleContext{
            .executor_ = context.executor_,
            .selection_commands_ = context.selection_commands_,
            .ui_command_registry_ = context.ui_command_registry_}));
    // «Свойства».
    context.singletons_.emplace(
        std::make_shared<NodePropertyModule>(NodePropertyModuleContext{
            .executor_ = context.executor_,
            .session_service_ = *context.scada_services_.session_service,
            .selection_commands_ = context.selection_commands_,
            .ui_command_registry_ = context.ui_command_registry_}));
    // «Копировать», «Вставить», «Удалить».
    context.singletons_.emplace(
        std::make_shared<SelectionEditModule>(SelectionEditModuleContext{
            .executor_ = context.executor_,
            .session_service_ = *context.scada_services_.session_service,
            .node_service_ = context.node_service_,
            .task_manager_ = context.task_manager_,
            .selection_commands_ = context.selection_commands_,
            .ui_command_registry_ = context.ui_command_registry_,
            .opened_view_commands_ = context.opened_view_commands_}));
    // The «Создать» submenu, which is the whole subject of two captures.
    context.singletons_.emplace(
        std::make_shared<CreateModule>(CreateModuleContext{
            .node_service_ = context.node_service_,
            .ui_command_registry_ = context.ui_command_registry_,
            .opened_view_commands_ = context.opened_view_commands_}));

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

    context.singletons_.emplace(std::make_shared<NodeServiceProgressTracker>(
        context.executor_, context.node_service_, context.progress_host_));
  };
}
