#include "aui/qt/dialog_util.h"

#include "aui/qt/dialog_test_util.h"
#include "aui/test/app_environment.h"

#include <QDialog>
#include <gtest/gtest.h>

#include <stdexcept>

namespace {

class DialogUtilTest : public testing::Test {
 protected:
  AppEnvironment app_env_;
};

}  // namespace

TEST_F(DialogUtilTest, DialogTestUtilDoesNotInvokeActionForSettledPromise) {
  auto result = make_resolved_promise(7);
  int action_count = 0;

  aui::qt::test::ProcessEventsUntilSettled(
      result, [&](QDialog&) { ++action_count; });

  EXPECT_TRUE(aui::qt::test::IsPromiseReady(result));
  EXPECT_EQ(result.get(), 7);
  EXPECT_EQ(action_count, 0);
}

TEST_F(DialogUtilTest, StartMappedModalDialogReturnsAcceptedMappedResult) {
  auto result = StartMappedModalDialog(std::make_unique<QDialog>(),
                                       [](QDialog& dialog) {
                                         return dialog.isModal() ? 42 : 0;
                                       });

  aui::qt::test::ProcessEventsUntilSettled(result,
                                           aui::qt::test::AcceptDialog);

  ASSERT_TRUE(aui::qt::test::IsPromiseReady(result));
  EXPECT_EQ(result.get(), 42);
}

TEST_F(DialogUtilTest, StartMappedModalDialogRejectsCanceledDialog) {
  auto result =
      StartMappedModalDialog(std::make_unique<QDialog>(),
                             [](QDialog& dialog) { return 42; });

  aui::qt::test::ProcessEventsUntilSettled(result,
                                           aui::qt::test::RejectDialog);

  ASSERT_TRUE(aui::qt::test::IsPromiseReady(result));
  EXPECT_THROW(result.get(), std::exception);
}

TEST_F(DialogUtilTest, StartMappedModalDialogRejectsMapperException) {
  auto result = StartMappedModalDialog(std::make_unique<QDialog>(),
                                       [](QDialog& dialog) -> int {
                                         throw std::runtime_error{"map"};
                                       });

  aui::qt::test::ProcessEventsUntilSettled(result,
                                           aui::qt::test::AcceptDialog);

  ASSERT_TRUE(aui::qt::test::IsPromiseReady(result));
  EXPECT_THROW(result.get(), std::runtime_error);
}
