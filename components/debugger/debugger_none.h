#pragma once

#include "components/debugger/debugger_context.h"

class CommandRegistry;

class Debugger {
 public:
  explicit Debugger(DebuggerContext&& context) {}

  void Open() {}

  void RegisterCommands(CommandRegistry& global_commands) {}
};