#include "export/csv/csv_export.h"

#include "aui/dialog_service.h"
#include "aui/qt/dialog_test_util.h"
#include "aui/test/app_environment.h"
#include "base/value_util.h"
#include "export/csv/csv_export_util.h"
#include "profile/profile.h"

#include <QComboBox>
#include <QDialog>
#include <gtest/gtest.h>

#include <filesystem>

namespace {

class TestDialogService : public DialogService {
 public:
  UiView* GetDialogOwningWindow() const override { return nullptr; }
  UiView* GetParentWidget() const override { return nullptr; }

  promise<MessageBoxResult> RunMessageBox(std::u16string_view message,
                                          std::u16string_view title,
                                          MessageBoxMode mode) override {
    return make_rejected_promise<MessageBoxResult>(std::exception{});
  }

  promise<std::filesystem::path> SelectOpenFile(
      std::u16string_view title) override {
    return make_rejected_promise<std::filesystem::path>(std::exception{});
  }

  promise<std::filesystem::path> SelectSaveFile(
      const SaveParams& params) override {
    return make_rejected_promise<std::filesystem::path>(std::exception{});
  }
};

CsvExportParams ReadProfileParams(Profile& profile) {
  return FromJson<CsvExportParams>(GetKey(profile.data(), "csv")).value();
}

void ExpectParamsEq(const CsvExportParams& actual,
                    const CsvExportParams& expected) {
  EXPECT_EQ(actual.unicode, expected.unicode);
  EXPECT_EQ(actual.delimiter, expected.delimiter);
  EXPECT_EQ(actual.quote, expected.quote);
}

class CsvExportDialogTest : public testing::Test {
 protected:
  AppEnvironment app_env_;
  TestDialogService dialog_service_;
  Profile profile_;
};

}  // namespace

TEST_F(CsvExportDialogTest, AcceptedDialogReturnsParamsAndStoresProfile) {
  auto result = ShowCsvExportDialog(dialog_service_, profile_);

  aui::qt::test::ProcessEventsUntilSettled(result, [](QDialog& dialog) {
    dialog.findChild<QComboBox*>("encodingComboBox")->setCurrentIndex(1);
    dialog.findChild<QComboBox*>("delimiterComboBox")->setCurrentText(";");
    dialog.findChild<QComboBox*>("quoteComboBox")->setCurrentText("'");
    dialog.accept();
  });

  ASSERT_TRUE(aui::qt::test::IsPromiseReady(result));
  const CsvExportParams expected{.unicode = true,
                                 .delimiter = ';',
                                 .quote = '\''};
  ExpectParamsEq(result.get(), expected);
  ExpectParamsEq(ReadProfileParams(profile_), expected);
}

TEST_F(CsvExportDialogTest, RejectedDialogDoesNotStoreProfileParams) {
  profile_.data().as_object()["csv"] =
      ToJson(CsvExportParams{.unicode = true, .delimiter = ';', .quote = '\''});

  auto result = ShowCsvExportDialog(dialog_service_, profile_);

  aui::qt::test::ProcessEventsUntilSettled(result,
                                           aui::qt::test::RejectDialog);

  ASSERT_TRUE(aui::qt::test::IsPromiseReady(result));
  EXPECT_THROW(result.get(), std::exception);
  ExpectParamsEq(ReadProfileParams(profile_),
                 CsvExportParams{.unicode = true,
                                 .delimiter = ';',
                                 .quote = '\''});
}
