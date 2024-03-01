#pragma once

#include "controller/command_registry.h"
#include "core/main_command_context.h"
#include "core/selection_command_context.h"

#include <memory>
#include <stack>

class Executor;
class Tracer;

class CoreModule {
 public:
  explicit CoreModule(std::shared_ptr<Executor> executor);

  Tracer& tracer() { return *tracer_; }

  BasicCommandRegistry<MainCommandContext>& main_commands() {
    return main_commands_;
  }

  BasicCommandRegistry<SelectionCommandContext>& selection_commands() {
    return selection_commands_;
  }

 private:
  std::stack<std::shared_ptr<void>> singletons_;

  BasicCommandRegistry<MainCommandContext> main_commands_;
  BasicCommandRegistry<SelectionCommandContext> selection_commands_;

  std::shared_ptr<Tracer> tracer_;
};