#include "export/configuration/export_configuration_module.h"

#include "common_resources.h"
#include "controller/command_registry.h"
#include "export/configuration/excel_configuration_commands.h"

ExportConfigurationModule::ExportConfigurationModule(
    ExportConfigurationModuleContext&& context)
    : ExportConfigurationModuleContext{std::move(context)} {
  /* main_commands_.registry().AddCommand(
      Command{ID_EXPORT_CONFIGURATION_TO_EXCEL}.set_execute_handler(
          [this, &context = main_commands_.context()] {
            ExportConfigurationToExcel(node_service_, context.dialog_service);
          }));

  main_commands_.registry().AddCommand(
      Command{ID_IMPORT_CONFIGURATION_FROM_EXCEL}.set_execute_handler(
          [this, &context = main_commands_.context()] {
            ImportConfigurationFromExcel(node_service_, task_manager_,
                                         context.dialog_service);
          }));*/
}
