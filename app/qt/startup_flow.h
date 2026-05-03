#pragma once

#include "base/any_executor.h"

#include "base/awaitable.h"

#include <functional>
#include <memory>


namespace client {

struct QtStartupFlowContext {
  AnyExecutor executor;

  std::function<Awaitable<void>()> start;
  std::function<Awaitable<void>()> run_object_view_values_check;
  std::function<Awaitable<void>()> run_operator_use_case_smoke;
  std::function<Awaitable<void>()> run_object_tree_labels_check;
  std::function<Awaitable<void>()> run_hardware_tree_devices_check;
  std::function<Awaitable<void>()> run_application;

  std::function<void(std::exception_ptr)> log_startup_exception;
  std::function<void()> report_startup_success_if_unset;
  std::function<void()> report_startup_failure_if_unset;
  std::function<void()> on_e2e_run_completed;
  std::function<void()> quit_application;

  bool e2e_test_mode = false;
};

Awaitable<void> RunQtStartupFlow(QtStartupFlowContext context);

}  // namespace client
