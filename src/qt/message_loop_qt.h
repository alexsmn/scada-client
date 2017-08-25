#pragma once

#include <QtCore/qtimer.h>

#include "base/pending_task.h"
#include "base/single_thread_task_runner.h"
#include "base/threading/thread_task_runner_handle.h"

class MessageLoopQt : public base::SingleThreadTaskRunner {
 public:
  MessageLoopQt();

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

  int sequence_num_;

  std::priority_queue<base::PendingTask> queue_;
  base::ThreadTaskRunnerHandle thread_task_runner_handle_;

  QTimer timer_;
};
