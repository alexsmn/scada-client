#pragma once

#include "base/qt/message_loop_qt.h"
#include "base/threading/thread_task_runner_handle.h"

#include <QApplication>

class AppEnvironment {
 private:
  int argc_ = 0;
  QApplication app_{argc_, nullptr};

  // QApplication must be created.
  base::ThreadTaskRunnerHandle task_runner_handle_{
      base::MakeRefCounted<MessageLoopQt>()};
};
