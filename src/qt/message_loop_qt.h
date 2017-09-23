#pragma once

#include <mutex>
#include <QtCore/qtimer.h>

#include "base/pending_task.h"
#include "base/single_thread_task_runner.h"

class MessageLoopQt : public base::SingleThreadTaskRunner {
 public:
  MessageLoopQt();
  ~MessageLoopQt();

  void Run();

  virtual bool PostDelayedTask(const tracked_objects::Location& from_here,
                               const base::Closure& task,
                               base::TimeDelta delay) override {
    return PostTaskHelper(from_here, task, delay, false);
  }

  virtual bool RunsTasksOnCurrentThread() const override {
    return true;
  }

  virtual bool PostNonNestableDelayedTask(
      const tracked_objects::Location& from_here,
      const base::Closure& task,
      base::TimeDelta delay) override {
    return PostTaskHelper(from_here, task, delay, false);
  }

 private:
  bool PostTaskHelper(const tracked_objects::Location& from_here,
                      const base::Closure& task, base::TimeDelta delay,
                      bool nestable);

  QTimer timer_;

  std::recursive_mutex mutex_;
  int sequence_num_ = 0;
  base::DelayedTaskQueue queue_;
};
