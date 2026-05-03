#pragma once

#include "base/any_executor.h"
#include "base/any_executor_adapter.h"
#include "base/common_types.h"

#include <QTimer>
#include <functional>
#include <mutex>
#include <queue>
#include <source_location>

class MessageLoopQt final {
 public:
  using Task = std::function<void()>;

  MessageLoopQt();
  ~MessageLoopQt();

  void PostDelayedTask(Duration delay,
                       Task task,
                       const std::source_location& location =
                           std::source_location::current());
  size_t GetTaskCount() const;

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
