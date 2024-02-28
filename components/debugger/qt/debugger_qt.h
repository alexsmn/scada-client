#pragma once

#include "components/debugger/debugger_context.h"

class QWidget;

class Debugger : private DebuggerContext {
 public:
  explicit Debugger(DebuggerContext&& context);

  void Open();

 private:
  QWidget* CreateRequestView(QWidget* parent);
};
