#include "export/excel/excel_export_module.h"

#include "aui/translation.h"
#include "controller/action.h"
#include "controller/command_ui_registry.h"
#include "resources/common_resources.h"

ExcelExportModule::ExcelExportModule(ExcelExportModuleContext&& context)
    : ExcelExportModuleContext{std::move(context)} {
  ui_command_registry_.AddAction(
      Action{.command_id_ = ID_EXPORT_EXCEL,
             .category_ = CATEGORY_EXPORT,
             .title_ = Translate("Export to Excel")});
}
