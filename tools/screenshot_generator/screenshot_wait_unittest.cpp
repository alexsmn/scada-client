#include "screenshot_wait.h"

#include "aui/qt/message_loop_qt.h"
#include "aui/test/app_environment.h"

#include <gtest/gtest.h>

#include <stdexcept>

namespace screenshot_generator {

namespace {

Awaitable<int> ResolveInt() {
  co_return 42;
}

Awaitable<int> RejectInt() {
  throw std::runtime_error{"value failure"};
  co_return 0;
}

Awaitable<void> ResolveVoid() {
  co_return;
}

Awaitable<void> RejectVoid() {
  throw std::runtime_error{"void failure"};
  co_return;
}

AnyExecutor MakeExecutor() {
  return MakeAnyExecutor(std::make_shared<MessageLoopQt>());
}

}  // namespace

class ScreenshotWaitTest : public testing::Test {
 protected:
  AppEnvironment app_env_;
};

TEST_F(ScreenshotWaitTest, WaitForAwaitableReturnsResolvedValue) {
  EXPECT_EQ(WaitForAwaitable(MakeExecutor(), ResolveInt()), 42);
}

TEST_F(ScreenshotWaitTest, WaitForAwaitablePropagatesRejectedValue) {
  EXPECT_THROW(WaitForAwaitable(MakeExecutor(), RejectInt()), std::runtime_error);
}

TEST_F(ScreenshotWaitTest, WaitForAwaitableCompletesResolvedVoid) {
  EXPECT_NO_THROW(WaitForAwaitable(MakeExecutor(), ResolveVoid()));
}

TEST_F(ScreenshotWaitTest, WaitForAwaitablePropagatesRejectedVoid) {
  EXPECT_THROW(WaitForAwaitable(MakeExecutor(), RejectVoid()), std::runtime_error);
}

}  // namespace screenshot_generator
