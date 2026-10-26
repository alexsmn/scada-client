#include "export/excel/opened_view_excel_export_command.h"

#include "aui/dialog_service_mock.h"
#include "aui/models/table_model.h"
#include "base/test/test_executor.h"
#include "export/export_model.h"
#include "resources/common_resources.h"

#include <gmock/gmock.h>

using namespace testing;

namespace {

class EmptyTableModel : public scada::aui::TableModel {
 public:
  int GetRowCount() override { return 0; }
  void GetCell(scada::aui::TableCell&) override {}
};

class TestExportModel : public ExportModel {
 public:
  ExportData GetExportData() override {
    return TableExportData{.model = table_model, .columns = columns};
  }

  EmptyTableModel table_model;
  std::vector<scada::aui::TableColumn> columns;
};

class OpenedViewExcelExportCommandTest : public Test {
 protected:
  OpenedViewExcelExportCommand MakeCommand(bool excel_available,
                                           ExportModel* export_model) {
    return OpenedViewExcelExportCommand{
        {.executor_ = executor_,
         .dialog_service_ = dialog_service_,
         .export_model_getter_ = [export_model] { return export_model; },
         .excel_available_ = [excel_available] { return excel_available; }}};
  }

  TestExecutor executor_;
  StrictMock<MockDialogService> dialog_service_;
  TestExportModel export_model_;
};

// Backlog 873: the command used to need the undocumented `--excel` switch. It
// is now offered exactly where Excel can be automated and the view exports.
TEST_F(OpenedViewExcelExportCommandTest, OfferedWhereExcelIsAvailable) {
  auto command = MakeCommand(/*excel_available=*/true, &export_model_);
  EXPECT_EQ(command.GetCommandHandler(ID_EXPORT_EXCEL), &command);
}

TEST_F(OpenedViewExcelExportCommandTest, NotOfferedWithoutExcel) {
  auto command = MakeCommand(/*excel_available=*/false, &export_model_);
  EXPECT_EQ(command.GetCommandHandler(ID_EXPORT_EXCEL), nullptr);
}

TEST_F(OpenedViewExcelExportCommandTest, NotOfferedForAViewThatDoesNotExport) {
  auto command = MakeCommand(/*excel_available=*/true, nullptr);
  EXPECT_EQ(command.GetCommandHandler(ID_EXPORT_EXCEL), nullptr);
}

#if !defined(_WIN32)
TEST_F(OpenedViewExcelExportCommandTest, ExcelIsNeverAvailableOffWindows) {
  EXPECT_FALSE(Excel::IsAvailable());
}
#endif

}  // namespace
