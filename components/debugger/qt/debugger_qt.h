#pragma once

#include "components/debugger/debugger_context.h"

#include <memory>

class QWidget;
class RequestTableModel;

class Debugger : private DebuggerContext {
 public:
  explicit Debugger(DebuggerContext&& context);

  void Open();

 private:
  QWidget* CreateRequestView(QWidget* parent);

  std::shared_ptr<RequestTableModel> request_table_model_;
};
