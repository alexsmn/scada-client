#include "aui/qt/dialog_util.h"

#include "aui/qt/dialog_test_util.h"
#include "aui/test/app_environment.h"

#include <QApplication>
#include <QDialog>
#include <QEventLoop>
#include <QPointer>
#include <gtest/gtest.h>

#include <chrono>
#include <stdexcept>
#include <thread>

namespace {

class DialogUtilTest : public testing::Test {
 protected:
  AppEnvironment app_env_;
};

template <class T>
Awaitable<T> MakeResolvedAwaitable(T value) {
  co_return std::move(value);
}

template <typename Predicate>
void ProcessEventsUntil(Predicate predicate) {
  for (int i = 0; i < 200 && !predicate(); ++i) {
    QApplication::processEvents(QEventLoop::AllEvents |
                                    QEventLoop::WaitForMoreEvents,
                                20);
    std::this_thread::sleep_for(std::chrono::milliseconds{1});
  }
}

}  // namespace

TEST_F(DialogUtilTest, DialogTestUtilDoesNotInvokeActionForSettledPromise) {
  auto result = aui::qt::test::StartAwaitable(MakeResolvedAwaitable(7));
  int action_count = 0;

  aui::qt::test::ProcessEventsUntilSettled(
      result, [&](QDialog&) { ++action_count; });

  EXPECT_TRUE(aui::qt::test::IsAwaitableReady(result));
  EXPECT_EQ(aui::qt::test::GetAwaitableResult(result), 7);
  EXPECT_EQ(action_count, 0);
}

TEST_F(DialogUtilTest, StartMappedModalDialogReturnsAcceptedMappedResult) {
  auto result =
      aui::qt::test::StartAwaitable(StartMappedModalDialog(
          std::make_unique<QDialog>(),
          [](QDialog& dialog) { return dialog.isModal() ? 42 : 0; }));

  aui::qt::test::ProcessEventsUntilSettled(result,
                                           aui::qt::test::AcceptDialog);

  ASSERT_TRUE(aui::qt::test::IsAwaitableReady(result));
  EXPECT_EQ(aui::qt::test::GetAwaitableResult(result), 42);
}

TEST_F(DialogUtilTest, StartOwnedModalDialogShowsDialogUntilAccepted) {
  auto dialog = std::make_unique<QDialog>();
  QPointer<QDialog> dialog_ptr = dialog.get();

  auto result = aui::qt::test::StartAwaitable(
      StartOwnedModalDialog(std::move(dialog)));

  ProcessEventsUntil([&] { return dialog_ptr && dialog_ptr->isVisible(); });
  ASSERT_TRUE(dialog_ptr);
  EXPECT_TRUE(dialog_ptr->isModal());
  EXPECT_TRUE(dialog_ptr->isVisible());

  aui::qt::test::AcceptDialog(*dialog_ptr);
  aui::qt::test::ProcessEventsUntilSettled(result, [](QDialog&) {});

  ASSERT_TRUE(aui::qt::test::IsAwaitableReady(result));
  EXPECT_NO_THROW(aui::qt::test::GetAwaitableResult(result));
}

TEST_F(DialogUtilTest, StartOwnedModalDialogRejectsCanceledDialog) {
  auto dialog = std::make_unique<QDialog>();
  QPointer<QDialog> dialog_ptr = dialog.get();

  auto result = aui::qt::test::StartAwaitable(
      StartOwnedModalDialog(std::move(dialog)));

  ProcessEventsUntil([&] { return dialog_ptr && dialog_ptr->isVisible(); });
  ASSERT_TRUE(dialog_ptr);
  EXPECT_TRUE(dialog_ptr->isVisible());

  aui::qt::test::RejectDialog(*dialog_ptr);
  aui::qt::test::ProcessEventsUntilSettled(result, [](QDialog&) {});

  ASSERT_TRUE(aui::qt::test::IsAwaitableReady(result));
  EXPECT_THROW(aui::qt::test::GetAwaitableResult(result), std::exception);
}

TEST_F(DialogUtilTest, StartMappedModalDialogRejectsCanceledDialog) {
  auto result = aui::qt::test::StartAwaitable(StartMappedModalDialog(
      std::make_unique<QDialog>(), [](QDialog& dialog) { return 42; }));

  aui::qt::test::ProcessEventsUntilSettled(result,
                                           aui::qt::test::RejectDialog);

  ASSERT_TRUE(aui::qt::test::IsAwaitableReady(result));
  EXPECT_THROW(aui::qt::test::GetAwaitableResult(result), std::exception);
}

TEST_F(DialogUtilTest, StartMappedModalDialogRejectsMapperException) {
  auto result =
      aui::qt::test::StartAwaitable(StartMappedModalDialog(
          std::make_unique<QDialog>(), [](QDialog& dialog) -> int {
            throw std::runtime_error{"map"};
          }));

  aui::qt::test::ProcessEventsUntilSettled(result,
                                           aui::qt::test::AcceptDialog);

  ASSERT_TRUE(aui::qt::test::IsAwaitableReady(result));
  EXPECT_THROW(aui::qt::test::GetAwaitableResult(result), std::runtime_error);
}

TEST_F(DialogUtilTest, StartFinishedModalDialogReturnsMappedResult) {
  auto result =
      aui::qt::test::StartAwaitable(StartFinishedModalDialog(
          std::make_unique<QDialog>(),
          [](QDialog& dialog, int finished_result) {
            return dialog.isModal() ? finished_result : 0;
          }));

  aui::qt::test::ProcessEventsUntilSettled(result,
                                           aui::qt::test::AcceptDialog);

  ASSERT_TRUE(aui::qt::test::IsAwaitableReady(result));
  EXPECT_EQ(aui::qt::test::GetAwaitableResult(result), QDialog::Accepted);
}

TEST_F(DialogUtilTest, StartFinishedModalDialogRejectsMapperException) {
  auto result =
      aui::qt::test::StartAwaitable(StartFinishedModalDialog(
          std::make_unique<QDialog>(),
          [](QDialog& dialog, int finished_result) -> int {
            throw std::runtime_error{"map"};
          }));

  aui::qt::test::ProcessEventsUntilSettled(result,
                                           aui::qt::test::RejectDialog);

  ASSERT_TRUE(aui::qt::test::IsAwaitableReady(result));
  EXPECT_THROW(aui::qt::test::GetAwaitableResult(result), std::runtime_error);
}
