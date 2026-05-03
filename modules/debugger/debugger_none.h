#pragma once

#include "modules/debugger/debugger_context.h"

class ActionManager;

class Debugger {
 public:
  explicit Debugger(DebuggerContext&& context) {}

  void Open() {}

  void RegisterCommands(ActionManager& action_manager) {}
};
