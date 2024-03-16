#pragma once

#include "controller/command_registry.h"
#include "core/global_command_context.h"
#include "core/selection_command_context.h"

#include <memory>
#include <stack>

class Executor;
class Tracer;

class CoreModule {
 public:
  explicit CoreModule(std::shared_ptr<Executor> executor);
  ~CoreModule();

  Tracer& tracer() { return *tracer_; }

  BasicCommandRegistry<GlobalCommandContext>& global_commands() {
    return global_commands_;
  }

  BasicCommandRegistry<SelectionCommandContext>& selection_commands() {
    return selection_commands_;
  }

 private:
  std::stack<std::shared_ptr<void>> singletons_;

  std::unique_ptr<Tracer> tracer_;

  BasicCommandRegistry<GlobalCommandContext> global_commands_;
  BasicCommandRegistry<SelectionCommandContext> selection_commands_;
};