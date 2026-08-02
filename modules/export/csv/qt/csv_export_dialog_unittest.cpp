#include "export/csv/csv_export.h"

#include "aui/dialog_service.h"
#include "aui/qt/dialog_test_util.h"
#include "aui/test/app_environment.h"
#include "base/value_util.h"
#include "export/csv/csv_export_util.h"
#include "profile/profile.h"

#include <QCheckBox>
#include <QComboBox>
#include <QDialog>
#include <gtest/gtest.h>

#include <filesystem>

namespace {

class TestDialogService : public DialogService {
 public:
  UiView* GetDialogOwningWindow() const override { return nullptr; }
  UiView* GetParentWidget() const override { return nullptr; }

  Awaitable<MessageBoxResult> RunMessageBox(std::u16string_view message,
                                            std::u16string_view title,
                                            MessageBoxMode mode) override {
    throw std::exception{};
    co_return MessageBoxResult::Ok;
  }

  Awaitable<std::filesystem::path> SelectOpenFile(
      std::u16string_view title) override {
    throw std::exception{};
    co_return std::filesystem::path{};
  }

  Awaitable<std::filesystem::path> SelectSaveFile(
      const SaveParams& params) override {
    throw std::exception{};
    co_return std::filesystem::path{};
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
  EXPECT_EQ(actual.expand_groups, expected.expand_groups);
}

class CsvExportDialogTest : public testing::Test {
 protected:
  AppEnvironment app_env_;
  TestDialogService dialog_service_;
  Profile profile_;
};

}  // namespace

TEST_F(CsvExportDialogTest, AcceptedDialogReturnsParamsAndStoresProfile) {
  auto result = scada::aui::qt::test::StartAwaitable(
      ShowCsvExportDialog(dialog_service_, profile_, /*can_expand=*/false));

  scada::aui::qt::test::ProcessEventsUntilSettled(result, [](QDialog& dialog) {
    dialog.findChild<QComboBox*>("encodingComboBox")->setCurrentIndex(1);
    dialog.findChild<QComboBox*>("delimiterComboBox")->setCurrentText(";");
    dialog.findChild<QComboBox*>("quoteComboBox")->setCurrentText("'");
    dialog.accept();
  });

  ASSERT_TRUE(scada::aui::qt::test::IsAwaitableReady(result));
  const CsvExportParams expected{
      .unicode = true, .delimiter = ';', .quote = '\''};
  ExpectParamsEq(scada::aui::qt::test::GetAwaitableResult(result), expected);
  ExpectParamsEq(ReadProfileParams(profile_), expected);
}

TEST_F(CsvExportDialogTest, RejectedDialogDoesNotStoreProfileParams) {
  profile_.data().as_object()["csv"] =
      ToJson(CsvExportParams{.unicode = true, .delimiter = ';', .quote = '\''});

  auto result = scada::aui::qt::test::StartAwaitable(
      ShowCsvExportDialog(dialog_service_, profile_, /*can_expand=*/false));

  scada::aui::qt::test::ProcessEventsUntilSettled(
      result, scada::aui::qt::test::RejectDialog);

  ASSERT_TRUE(scada::aui::qt::test::IsAwaitableReady(result));
  EXPECT_THROW(scada::aui::qt::test::GetAwaitableResult(result),
               std::exception);
  ExpectParamsEq(
      ReadProfileParams(profile_),
      CsvExportParams{.unicode = true, .delimiter = ';', .quote = '\''});
}

// The expand option is only meaningful for a view that groups rows, so it is
// hidden rather than shown with no effect.
TEST_F(CsvExportDialogTest, ExpandOptionIsHiddenWhenThereIsNothingToExpand) {
  auto result = scada::aui::qt::test::StartAwaitable(
      ShowCsvExportDialog(dialog_service_, profile_, /*can_expand=*/false));

  scada::aui::qt::test::ProcessEventsUntilSettled(result, [](QDialog& dialog) {
    auto* checkbox = dialog.findChild<QCheckBox*>("expandGroupsCheckBox");
    ASSERT_NE(checkbox, nullptr);
    EXPECT_FALSE(checkbox->isVisible());
    dialog.accept();
  });

  ASSERT_TRUE(scada::aui::qt::test::IsAwaitableReady(result));
  // Hidden means untouched: the stored preference is not rewritten by a view
  // that could not honour it either way.
  EXPECT_TRUE(scada::aui::qt::test::GetAwaitableResult(result).expand_groups);
}

TEST_F(CsvExportDialogTest, ExpandOptionIsOfferedAndStoredWhenGroupsExist) {
  auto result = scada::aui::qt::test::StartAwaitable(
      ShowCsvExportDialog(dialog_service_, profile_, /*can_expand=*/true));

  scada::aui::qt::test::ProcessEventsUntilSettled(result, [](QDialog& dialog) {
    auto* checkbox = dialog.findChild<QCheckBox*>("expandGroupsCheckBox");
    ASSERT_NE(checkbox, nullptr);
    EXPECT_TRUE(checkbox->isVisible());
    // Defaults to expanding — a spreadsheet is a record of what happened.
    EXPECT_TRUE(checkbox->isChecked());
    checkbox->setChecked(false);
    dialog.accept();
  });

  ASSERT_TRUE(scada::aui::qt::test::IsAwaitableReady(result));
  EXPECT_FALSE(scada::aui::qt::test::GetAwaitableResult(result).expand_groups);
  // And the choice is remembered for the next export.
  EXPECT_FALSE(ReadProfileParams(profile_).expand_groups);
}
