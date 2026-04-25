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

class QtStartupFlowTest : public testing::Test {
 protected:
  QtStartupFlowContext MakeContext() {
    return QtStartupFlowContext{
        .executor = executor_,
        .start = [this] {
          order_.push_back("start");
          return make_resolved_promise();
        },
        .run_object_view_values_check =
            [this] {
              order_.push_back("object-view");
              return make_resolved_promise();
            },
        .run_operator_use_case_smoke =
            [this] {
              order_.push_back("operator-smoke");
              return make_resolved_promise();
            },
        .run_object_tree_labels_check =
            [this] {
              order_.push_back("object-tree");
              return make_resolved_promise();
            },
        .run_hardware_tree_devices_check =
            [this] {
              order_.push_back("hardware-tree");
              return make_resolved_promise();
            },
        .run_application =
            [this] {
              order_.push_back("run");
              return make_resolved_promise();
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

  std::shared_ptr<TestExecutor> executor_ = std::make_shared<TestExecutor>();
  std::vector<std::string> order_;
  bool logged_exception_ = false;
};

}  // namespace

TEST_F(QtStartupFlowTest, SuccessfulNormalStartupRunsChecksAndQuits) {
  auto context = MakeContext();

  WaitPromise(executor_, RunQtStartupFlow(std::move(context)));
  Drain(executor_);

  EXPECT_THAT(order_, testing::ElementsAre("start",
                                           "object-view",
                                           "operator-smoke",
                                           "success",
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
    return make_rejected_promise<void>(std::runtime_error{"start failed"});
  };

  WaitPromise(executor_, RunQtStartupFlow(std::move(context)));
  Drain(executor_);

  EXPECT_THAT(order_, testing::ElementsAre("start", "failure", "quit"));
  EXPECT_TRUE(logged_exception_);
}

TEST_F(QtStartupFlowTest, E2eModeReportsRunCompletionInsteadOfQuitting) {
  auto context = MakeContext();
  context.e2e_test_mode = true;

  WaitPromise(executor_, RunQtStartupFlow(std::move(context)));
  Drain(executor_);

  EXPECT_THAT(order_, testing::ElementsAre("start",
                                           "object-view",
                                           "operator-smoke",
                                           "success",
                                           "object-tree",
                                           "hardware-tree",
                                           "run",
                                           "e2e-complete"));
}

}  // namespace client
