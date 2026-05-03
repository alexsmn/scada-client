#pragma once

#include "base/any_executor.h"

#include "aui/qt/message_loop_qt.h"

#include <QApplication>

class AppEnvironment {
 private:
  int argc_ = 0;
  QApplication app_{argc_, nullptr};

  // QApplication must be created.
  AnyExecutor executor_ = MakeAnyExecutor(std::make_shared<MessageLoopQt>());
};
