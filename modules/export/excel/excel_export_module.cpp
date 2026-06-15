#include "export/excel/excel_export_module.h"

#include "aui/translation.h"
#include "controller/action.h"
#include "controller/command_ui_registry.h"
#include "export/excel/opened_view_excel_export_command.h"
#include "main_window/opened_view/opened_view_command_registry.h"
#include "resources/common_resources.h"

ExcelExportModule::ExcelExportModule(ExcelExportModuleContext&& context)
    : ExcelExportModuleContext{std::move(context)} {
  ui_command_registry_.AddAction(
      Action{.command_id_ = ID_EXPORT_EXCEL,
             .category_ = CATEGORY_EXPORT,
             .title_ = Translate("Export to Excel")});
  opened_view_commands_.AddFactory(
      [](const OpenedViewCommandFactoryContext& context) {
        return std::make_unique<OpenedViewExcelExportCommand>(
            OpenedViewExcelExportCommandContext{
                .dialog_service_ = context.dialog_service_,
                .export_model_getter_ = context.export_model_getter_});
      });
}
