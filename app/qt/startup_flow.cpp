#include "app/qt/startup_flow.h"

#include "base/awaitable.h"
#include "base/any_executor.h"
#include "base/boost_log.h"

#include <exception>
#include <utility>

namespace client {
namespace {

Awaitable<void> RunQtStartupFlowAsync(QtStartupFlowContext context) {
  try {
    co_await context.start();
    co_await context.run_object_view_values_check();
    co_await context.run_operator_use_case_smoke();

    BOOST_LOG_TRIVIAL(info)
        << "Client startup completed; entering steady-state run loop";
    context.report_startup_success_if_unset();

    co_await context.run_object_tree_labels_check();
    co_await context.run_hardware_tree_devices_check();

    co_await context.run_application();
  } catch (...) {
    context.log_startup_exception(std::current_exception());
    context.report_startup_failure_if_unset();
  }

  if (context.e2e_test_mode) {
    context.on_e2e_run_completed();
  } else {
    context.quit_application();
  }

  co_return;
}

}  // namespace

Awaitable<void> RunQtStartupFlow(QtStartupFlowContext context) {
  return RunQtStartupFlowAsync(std::move(context));
}

}  // namespace client
