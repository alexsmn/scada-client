
#include "screenshot_wait.h"

#include "aui/qt/message_loop_qt.h"
#include "aui/test/app_environment.h"

#include <gtest/gtest.h>

#include <memory>
#include <stdexcept>

namespace scada::screenshot_generator {

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

}  // namespace

class ScreenshotWaitTest : public testing::Test {
 protected:
  // The pump is owned by the fixture, and that is load-bearing rather than
  // tidiness. These tests used to build it inside a `MakeExecutor()` helper
  // returning `MakeAnyExecutor(std::make_shared<MessageLoopQt>())`, so the
  // only owner of the loop was the `AnyExecutor` the spawned coroutine
  // captured: completing the awaitable dropped the last reference from inside
  // `MessageLoopQt::Run()`, which then re-locked a destroyed `mutex_` and
  // aborted with `recursive_mutex lock failed: Invalid argument`. All four
  // cases died that way. The generator itself never had the problem -- it
  // passes the application's own executor, which outlives every task -- so
  // this was a fixture that did not resemble any caller. The underlying
  // re-entrancy in `Run()` is real and is filed separately; owning the loop
  // here is what a caller actually does.
  AppEnvironment app_env_;
  std::shared_ptr<MessageLoopQt> message_loop_ =
      std::make_shared<MessageLoopQt>();
  AnyExecutor executor_ = MakeAnyExecutor(message_loop_);
};

TEST_F(ScreenshotWaitTest, WaitForAwaitableReturnsResolvedValue) {
  EXPECT_EQ(WaitForAwaitable(executor_, ResolveInt()), 42);
}

TEST_F(ScreenshotWaitTest, WaitForAwaitablePropagatesRejectedValue) {
  EXPECT_THROW(WaitForAwaitable(executor_, RejectInt()), std::runtime_error);
}

TEST_F(ScreenshotWaitTest, WaitForAwaitableCompletesResolvedVoid) {
  EXPECT_NO_THROW(WaitForAwaitable(executor_, ResolveVoid()));
}

TEST_F(ScreenshotWaitTest, WaitForAwaitablePropagatesRejectedVoid) {
  EXPECT_THROW(WaitForAwaitable(executor_, RejectVoid()), std::runtime_error);
}

}  // namespace scada::screenshot_generator
