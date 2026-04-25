#include "export/csv/csv_export_command.h"

#include "aui/dialog_service_mock.h"
#include "aui/models/table_model.h"
#include "base/test/awaitable_test.h"
#include "base/test/test_executor.h"
#include "base/value_util.h"
#include "export/export_model.h"
#include "profile/profile.h"

#include <chrono>
#include <filesystem>
#include <fstream>
#include <gmock/gmock.h>

#include "base/debug_util.h"

using namespace testing;

namespace {

class TestTableModel : public aui::TableModel {
 public:
  int GetRowCount() override { return 1; }

  void GetCell(aui::TableCell& cell) override {
    if (cell.row == 0 && cell.column_id == 1)
      cell.text = u"value";
  }
};

class TestExportModel : public ExportModel {
 public:
  ExportData GetExportData() override {
    if (throw_on_export)
      throw std::runtime_error{"export failed"};

    return TableExportData{.model = table_model, .columns = columns};
  }

  TestTableModel table_model;
  std::vector<aui::TableColumn> columns{{.id = 1, .title = u"Name"}};
  bool throw_on_export = false;
};

class CsvExportCommandTest : public Test {
 public:
  void SetUp() override {
    temp_dir_ = std::filesystem::temp_directory_path() /
                ("scada_csv_export_test_" +
                 std::to_string(std::chrono::steady_clock::now()
                                    .time_since_epoch()
                                    .count()));
    std::filesystem::create_directories(temp_dir_);
  }

  void TearDown() override { std::filesystem::remove_all(temp_dir_); }

 protected:
  promise<void> Run() {
    return RunCsvExport({.executor_ = executor_,
                         .dialog_service_ = dialog_service_,
                         .profile_ = profile_,
                         .export_model_ = export_model_,
                         .window_title_ = u"Test: Window",
                         .show_csv_export_dialog = show_csv_export_dialog_});
  }

  std::string ReadFile(const std::filesystem::path& path) {
    std::ifstream stream{path, std::ios::binary};
    return {std::istreambuf_iterator<char>{stream},
            std::istreambuf_iterator<char>{}};
  }

  std::shared_ptr<TestExecutor> executor_ = std::make_shared<TestExecutor>();
  StrictMock<MockDialogService> dialog_service_;
  Profile profile_;
  TestExportModel export_model_;
  CsvExportDialogRunner show_csv_export_dialog_ =
      [](DialogService&, Profile&) {
        return make_resolved_promise(CsvExportParams{});
      };
  std::filesystem::path temp_dir_;
};

}  // namespace

TEST_F(CsvExportCommandTest, WritesCsvAndPromptsToOpen) {
  const auto export_file_path = temp_dir_ / "export.csv";

  EXPECT_CALL(dialog_service_, SelectSaveFile(_))
      .WillOnce(DoAll(
          WithArg<0>([&](const DialogService::SaveParams& params) {
            EXPECT_EQ(params.title, u"Export");
            EXPECT_EQ(params.default_path.filename().u16string(),
                      u"Test- Window.csv");
          }),
          Return(make_resolved_promise(export_file_path))));

  EXPECT_CALL(dialog_service_,
              RunMessageBox(
                  std::u16string_view{u"Export completed. Open the file now?"},
                  std::u16string_view{u"Export"},
                  MessageBoxMode::QuestionYesNo))
      .WillOnce(Return(make_resolved_promise(MessageBoxResult::No)));

  WaitPromise(executor_, Run());

  EXPECT_EQ(GetString16(profile_.data(), "csvPath"),
            export_file_path.u16string());
  EXPECT_EQ(ReadFile(export_file_path), "Name\r\nvalue");
}

TEST_F(CsvExportCommandTest, ExportFailureShowsErrorDialogAndRejects) {
  const auto export_file_path = temp_dir_ / "export.csv";
  export_model_.throw_on_export = true;

  EXPECT_CALL(dialog_service_, SelectSaveFile(_))
      .WillOnce(Return(make_resolved_promise(export_file_path)));

  EXPECT_CALL(dialog_service_,
              RunMessageBox(std::u16string_view{u"Export failed."},
                            std::u16string_view{u"Export"},
                            MessageBoxMode::Error))
      .WillOnce(Return(make_resolved_promise(MessageBoxResult::Ok)));

  EXPECT_THROW(WaitPromise(executor_, Run()), std::runtime_error);
  EXPECT_FALSE(std::filesystem::exists(export_file_path));
}

TEST_F(CsvExportCommandTest, RejectedSaveDialogStopsExportFlow) {
  bool export_dialog_shown = false;
  show_csv_export_dialog_ = [&](DialogService&, Profile&) {
    export_dialog_shown = true;
    return make_resolved_promise(CsvExportParams{});
  };

  EXPECT_CALL(dialog_service_, SelectSaveFile(_))
      .WillOnce(Return(
          make_rejected_promise<std::filesystem::path>(std::exception{})));

  EXPECT_THROW(WaitPromise(executor_, Run()), std::exception);
  EXPECT_FALSE(export_dialog_shown);
}

TEST_F(CsvExportCommandTest, RejectedExportDialogStopsBeforeWritingFile) {
  const auto export_file_path = temp_dir_ / "export.csv";
  show_csv_export_dialog_ = [](DialogService&, Profile&) {
    return make_rejected_promise<CsvExportParams>(std::exception{});
  };

  EXPECT_CALL(dialog_service_, SelectSaveFile(_))
      .WillOnce(Return(make_resolved_promise(export_file_path)));

  EXPECT_THROW(WaitPromise(executor_, Run()), std::exception);
  EXPECT_FALSE(std::filesystem::exists(export_file_path));
}
