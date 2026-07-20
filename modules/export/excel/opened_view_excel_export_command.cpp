#include "export/excel/opened_view_excel_export_command.h"

#include "aui/dialog_service.h"
#include "base/check.h"
#include "base/excel.h"
#include "base/program_options.h"
#include "export/csv/csv_export_util.h"
#include "export/export_model.h"
#include "resources/common_resources.h"

OpenedViewExcelExportCommand::OpenedViewExcelExportCommand(
    OpenedViewExcelExportCommandContext&& context)
    : OpenedViewExcelExportCommandContext{std::move(context)} {}

CommandHandler* OpenedViewExcelExportCommand::GetCommandHandler(
    unsigned command_id) {
  return command_id == ID_EXPORT_EXCEL && client::HasOption("excel") &&
                 export_model_getter_()
             ? this
             : nullptr;
}

void OpenedViewExcelExportCommand::ExecuteCommand(unsigned command_id) {
  scada::base::Check(command_id == ID_EXPORT_EXCEL);
  auto* export_model = export_model_getter_();
  if (!export_model) {
    return;
  }

  auto export_data = export_model->GetExportData();

  try {
    ExcelSheetModel sheet;
    std::visit([&](auto& data) { ::ExportToExcel(data, sheet); }, export_data);

    Excel excel;
    excel.NewWorkbook();
    excel.NewSheet(sheet);
    excel.SetVisible();

  } catch (HRESULT /*err*/) {
    dialog_service_.RunMessageBox(
        u"Export failed. Please check that Microsoft Excel is installed "
        u"correctly.",
        u"Export", MessageBoxMode::Error);
  }
}
