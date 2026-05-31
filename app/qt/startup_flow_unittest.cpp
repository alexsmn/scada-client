#include "app/qt/startup_flow.h"

#include "base/test/awaitable_test.h"
#include "base/test/test_executor.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <stdexcept>
#include <string>
#include <vector>

namespace client {
namespace {

Awaitable<void> Resolve() {
  co_return;
}

Awaitable<void> Reject(std::exception_ptr error) {
  std::rethrow_exception(error);
  co_return;
}

class QtStartupFlowTest : public testing::Test {
 protected:
  QtStartupFlowContext MakeContext() {
    return QtStartupFlowContext{
        .executor = executor_,
        .start = [this] {
          order_.push_back("start");
          return Resolve();
        },
        .run_object_view_values_check =
            [this] {
              order_.push_back("object-view");
              return Resolve();
            },
        .run_operator_use_case_smoke =
            [this] {
              order_.push_back("operator-smoke");
              return Resolve();
            },
        .run_object_tree_labels_check =
            [this] {
              order_.push_back("object-tree");
              return Resolve();
            },
        .run_hardware_tree_devices_check =
            [this] {
              order_.push_back("hardware-tree");
              return Resolve();
            },
        .run_application =
            [this] {
              order_.push_back("run");
              return Resolve();
            },
        .log_startup_exception =
            [this](std::exception_ptr exception) {
              logged_exception_ = exception != nullptr;
            },
        .report_startup_success_if_unset =
            [this] { order_.push_back("success"); },
        .report_startup_failure_if_unset =
            [this] { order_.push_back("failure"); },
        .on_e2e_run_completed =
            [this] { order_.push_back("e2e-complete"); },
        .quit_application = [this] { order_.push_back("quit"); },
    };
  }

  TestExecutor executor_;
  std::vector<std::string> order_;
  bool logged_exception_ = false;
};

}  // namespace

TEST_F(QtStartupFlowTest, SuccessfulNormalStartupRunsChecksAndQuits) {
  auto context = MakeContext();

  WaitAwaitable(executor_, RunQtStartupFlow(std::move(context)));
  Drain(executor_);

  EXPECT_THAT(order_, testing::ElementsAre("start",
                                           "success",
                                           "object-view",
                                           "operator-smoke",
                                           "object-tree",
                                           "hardware-tree",
                                           "run",
                                           "quit"));
  EXPECT_FALSE(logged_exception_);
}

TEST_F(QtStartupFlowTest, StartupFailureReportsFailureAndQuits) {
  auto context = MakeContext();
  context.start = [this] {
    order_.push_back("start");
    return Reject(std::make_exception_ptr(std::runtime_error{"start failed"}));
  };

  WaitAwaitable(executor_, RunQtStartupFlow(std::move(context)));
  Drain(executor_);

  EXPECT_THAT(order_, testing::ElementsAre("start", "failure", "quit"));
  EXPECT_TRUE(logged_exception_);
}

TEST_F(QtStartupFlowTest, E2eModeReportsRunCompletionInsteadOfQuitting) {
  auto context = MakeContext();
  context.e2e_test_mode = true;

  WaitAwaitable(executor_, RunQtStartupFlow(std::move(context)));
  Drain(executor_);

  EXPECT_THAT(order_, testing::ElementsAre("start",
                                           "success",
                                           "object-view",
                                           "operator-smoke",
                                           "object-tree",
                                           "hardware-tree",
                                           "run",
                                           "e2e-complete"));
}

}  // namespace client
