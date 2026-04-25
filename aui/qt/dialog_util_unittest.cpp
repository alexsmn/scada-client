#include "aui/qt/dialog_util.h"

#include "aui/test/app_environment.h"

#include <QApplication>
#include <QDialog>
#include <QEventLoop>
#include <gtest/gtest.h>

#include <chrono>
#include <stdexcept>
#include <thread>

namespace {

template <class T>
void ProcessEventsUntilSettled(promise<T>& result, bool accept) {
  for (int i = 0; i < 200 &&
                  result.wait_for(std::chrono::milliseconds{0}) ==
                      promise_wait_status::timeout;
       ++i) {
    QApplication::processEvents(QEventLoop::AllEvents |
                                    QEventLoop::WaitForMoreEvents,
                                20);
    if (auto* dialog = qobject_cast<QDialog*>(QApplication::activeModalWidget())) {
      accept ? dialog->accept() : dialog->reject();
    }
    std::this_thread::sleep_for(std::chrono::milliseconds{1});
  }
}

class DialogUtilTest : public testing::Test {
 protected:
  AppEnvironment app_env_;
};

}  // namespace

TEST_F(DialogUtilTest, StartMappedModalDialogReturnsAcceptedMappedResult) {
  auto result = StartMappedModalDialog(std::make_unique<QDialog>(),
                                       [](QDialog& dialog) {
                                         return dialog.isModal() ? 42 : 0;
                                       });

  ProcessEventsUntilSettled(result, /*accept=*/true);

  ASSERT_NE(result.wait_for(std::chrono::milliseconds{0}),
            promise_wait_status::timeout);
  EXPECT_EQ(result.get(), 42);
}

TEST_F(DialogUtilTest, StartMappedModalDialogRejectsCanceledDialog) {
  auto result =
      StartMappedModalDialog(std::make_unique<QDialog>(),
                             [](QDialog& dialog) { return 42; });

  ProcessEventsUntilSettled(result, /*accept=*/false);

  ASSERT_NE(result.wait_for(std::chrono::milliseconds{0}),
            promise_wait_status::timeout);
  EXPECT_THROW(result.get(), std::exception);
}

TEST_F(DialogUtilTest, StartMappedModalDialogRejectsMapperException) {
  auto result = StartMappedModalDialog(std::make_unique<QDialog>(),
                                       [](QDialog& dialog) -> int {
                                         throw std::runtime_error{"map"};
                                       });

  ProcessEventsUntilSettled(result, /*accept=*/true);

  ASSERT_NE(result.wait_for(std::chrono::milliseconds{0}),
            promise_wait_status::timeout);
  EXPECT_THROW(result.get(), std::runtime_error);
}
