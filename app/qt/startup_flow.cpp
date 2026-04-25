#include "app/qt/startup_flow.h"

#include "base/awaitable.h"
#include "base/awaitable_promise.h"
#include "base/executor_conversions.h"
#include "base/boost_log.h"

#include <exception>
#include <utility>

namespace client {
namespace {

Awaitable<void> RunQtStartupFlowAsync(QtStartupFlowContext context) {
  try {
    co_await AwaitPromise(MakeAnyExecutor(context.executor), context.start());
    co_await AwaitPromise(MakeAnyExecutor(context.executor),
                          context.run_object_view_values_check());
    co_await AwaitPromise(MakeAnyExecutor(context.executor),
                          context.run_operator_use_case_smoke());

    BOOST_LOG_TRIVIAL(info)
        << "Client startup completed; entering steady-state run loop";
    context.report_startup_success_if_unset();

    co_await AwaitPromise(MakeAnyExecutor(context.executor),
                          context.run_object_tree_labels_check());
    co_await AwaitPromise(MakeAnyExecutor(context.executor),
                          context.run_hardware_tree_devices_check());

    co_await AwaitPromise(MakeAnyExecutor(context.executor),
                          context.run_application());
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

promise<void> RunQtStartupFlow(QtStartupFlowContext context) {
  auto executor = context.executor;
  return ToPromise(MakeAnyExecutor(std::move(executor)),
                   RunQtStartupFlowAsync(std::move(context)));
}

}  // namespace client
