#pragma once

#include "base/promise.h"

#include <functional>
#include <memory>

class Executor;

namespace client {

struct QtStartupFlowContext {
  std::shared_ptr<Executor> executor;

  std::function<promise<void>()> start;
  std::function<promise<void>()> run_object_view_values_check;
  std::function<promise<void>()> run_operator_use_case_smoke;
  std::function<promise<void>()> run_object_tree_labels_check;
  std::function<promise<void>()> run_hardware_tree_devices_check;
  std::function<promise<void>()> run_application;

  std::function<void(std::exception_ptr)> log_startup_exception;
  std::function<void()> report_startup_success_if_unset;
  std::function<void()> report_startup_failure_if_unset;
  std::function<void()> on_e2e_run_completed;
  std::function<void()> quit_application;

  bool e2e_test_mode = false;
};

promise<void> RunQtStartupFlow(QtStartupFlowContext context);

}  // namespace client
