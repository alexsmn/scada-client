#pragma once

#include "base/executor.h"

#include <QTimer>
#include <mutex>
#include <queue>

class MessageLoopQt final : public Executor {
 public:
  MessageLoopQt();
  ~MessageLoopQt();

  // Executor
  virtual void PostDelayedTask(Duration delay,
                               Task task,
                               const std::source_location& location =
                                   std::source_location::current()) override;
  virtual size_t GetTaskCount() const override;

 private:
  struct PendingTask {
    bool operator<(const PendingTask& other) const;

    Task task;
    TimePoint time;
    int sequence = 0;
  };

  void Run();

  QTimer timer_;

  mutable std::recursive_mutex mutex_;
  int sequence_num_ = 0;
  std::queue<Task> immediate_queue_;
  std::priority_queue<PendingTask> delayed_queue_;
};
