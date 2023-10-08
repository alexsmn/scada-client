#pragma once

#include "base/pending_task.h"
#include "base/single_thread_task_runner.h"

#include <QTimer>
#include <mutex>

class MessageLoopQt final : public base::SingleThreadTaskRunner {
 public:
  MessageLoopQt();
  ~MessageLoopQt();

  void Run();

  virtual bool PostDelayedTask(const base::Location& from_here,
                               base::OnceClosure task,
                               base::TimeDelta delay) override {
    return PostTaskHelper(from_here, std::move(task), delay,
                          base::Nestable::kNestable);
  }

  virtual bool PostNonNestableDelayedTask(const base::Location& from_here,
                                          base::OnceClosure task,
                                          base::TimeDelta delay) override {
    return PostTaskHelper(from_here, std::move(task), delay,
                          base::Nestable::kNonNestable);
  }

  virtual bool RunsTasksInCurrentSequence() const override { return true; }

 private:
  bool PostTaskHelper(const base::Location& from_here,
                      base::OnceClosure task,
                      base::TimeDelta delay,
                      base::Nestable nestable);

  QTimer timer_;

  std::recursive_mutex mutex_;
  int sequence_num_ = 0;
  base::DelayedTaskQueue queue_;
};
