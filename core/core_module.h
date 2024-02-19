#pragma once

#include "controller/command_registry.h"
#include "core/main_command_context.h"
#include "core/selection_command_context.h"

class CoreModule {
 public:
  BasicCommandRegistry<MainCommandContext>& main_commands() {
    return main_commands_;
  }

  BasicCommandRegistry<SelectionCommandContext>& selection_commands() {
    return selection_commands_;
  }

 private:
  BasicCommandRegistry<MainCommandContext> main_commands_;
  BasicCommandRegistry<SelectionCommandContext> selection_commands_;
};