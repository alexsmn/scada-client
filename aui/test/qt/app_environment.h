#pragma once

#include "aui/qt/message_loop_qt.h"

#include <QApplication>

class AppEnvironment {
 private:
  int argc_ = 0;
  QApplication app_{argc_, nullptr};

  // QApplication must be created.
  std::shared_ptr<Executor> executor_ = std::make_shared<MessageLoopQt>();
};
