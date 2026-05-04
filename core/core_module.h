#pragma once

#include "base/any_executor.h"

#include <memory>
#include <stack>

class ProgressHost;
class Tracer;

class CoreModule {
 public:
  explicit CoreModule(AnyExecutor executor);
  ~CoreModule();

  Tracer& tracer() { return *tracer_; }

  ProgressHost& progress_host() { return *progress_host_; }

 private:
  std::stack<std::shared_ptr<void>> singletons_;

  std::unique_ptr<Tracer> tracer_;

  std::unique_ptr<ProgressHost> progress_host_;
};
