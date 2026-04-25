#include "screenshot_wait.h"

#include <gtest/gtest.h>

#include <stdexcept>

namespace screenshot_generator {

TEST(ScreenshotWaitTest, WaitForPromiseReturnsResolvedValue) {
  EXPECT_EQ(WaitForPromise(make_resolved_promise(42)), 42);
}

TEST(ScreenshotWaitTest, WaitForPromisePropagatesRejectedValuePromise) {
  EXPECT_THROW(WaitForPromise(make_rejected_promise<int>(
                   std::runtime_error{"value failure"})),
               std::runtime_error);
}

TEST(ScreenshotWaitTest, WaitForPromiseCompletesResolvedVoidPromise) {
  EXPECT_NO_THROW(WaitForPromise(make_resolved_promise()));
}

TEST(ScreenshotWaitTest, WaitForPromisePropagatesRejectedVoidPromise) {
  EXPECT_THROW(WaitForPromise(make_rejected_promise<void>(
                   std::runtime_error{"void failure"})),
               std::runtime_error);
}

}  // namespace screenshot_generator
