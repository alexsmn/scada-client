#pragma once

#include "controller/command_registry.h"
#include "core/main_command_context.h"
#include "core/selection_command_context.h"

#include <memory>
#include <stack>

class Executor;
class TraceSpan;

class CoreModule {
 public:
  explicit CoreModule(std::shared_ptr<Executor> executor);
  ~CoreModule();

  const TraceSpan& root_trace_span() { return *root_trace_span_; }

  BasicCommandRegistry<MainCommandContext>& main_commands() {
    return main_commands_;
  }

  BasicCommandRegistry<SelectionCommandContext>& selection_commands() {
    return selection_commands_;
  }

 private:
  std::stack<std::shared_ptr<void>> singletons_;

  std::unique_ptr<TraceSpan> root_trace_span_;

  BasicCommandRegistry<MainCommandContext> main_commands_;
  BasicCommandRegistry<SelectionCommandContext> selection_commands_;
};