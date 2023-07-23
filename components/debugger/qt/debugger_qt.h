#pragma once

#include "components/debugger/debugger_context.h"

class CommandRegistry;
class QWidget;

class Debugger : private DebuggerContext {
 public:
  explicit Debugger(DebuggerContext&& context);

  void RegisterCommands(CommandRegistry& main_commands);

  void Open();

 private:
  QWidget* CreateRequestView(QWidget* parent);
};
