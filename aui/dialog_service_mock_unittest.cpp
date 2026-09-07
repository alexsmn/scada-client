#include "aui/dialog_service_mock.h"

#include "base/test/awaitable_test.h"
#include "base/test/test_executor.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <filesystem>

using namespace testing;

namespace {

// Each unstubbed `Awaitable`-returning method completes with a value rather
// than with gmock's default-constructed awaitable, which holds a null frame
// whose `await_ready()` still returns false — so `co_await`ing it dereferences
// null inside the *awaiting* coroutine, with this mock named nowhere in the
// backtrace (task 698; CLAUDE.md, "Unit Test Guidance").
//
// `NiceMock` is the case that matters: a `StrictMock` reports the unexpected
// call before it can return anything, so it never reaches the null frame.

TEST(DialogServiceMockTest, UnstubbedRunMessageBoxAnswersCancel) {
  TestExecutor executor;
  NiceMock<MockDialogService> service;

  MessageBoxResult result =
      WaitAwaitable(executor, service.RunMessageBox(u"message", u"title",
                                                    MessageBoxMode::Info));

  EXPECT_EQ(MessageBoxResult::Cancel, result);
}

TEST(DialogServiceMockTest, UnstubbedSelectSaveFileAnswersAnEmptyPath) {
  TestExecutor executor;
  NiceMock<MockDialogService> service;

  DialogService::SaveParams params{.title = u"title"};
  std::filesystem::path path =
      WaitAwaitable(executor, service.SelectSaveFile(params));

  EXPECT_TRUE(path.empty());
}

}  // namespace
