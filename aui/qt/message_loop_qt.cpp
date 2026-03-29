#include "aui/qt/message_loop_qt.h"

#include <cassert>

// MessageLoopQt::PendingTask

bool MessageLoopQt::PendingTask::operator<(const PendingTask& other) const {
  if (time != other.time)
    return time > other.time;  // min-heap: smaller time = higher priority
  return sequence > other.sequence;
}

// MessageLoopQt

MessageLoopQt::MessageLoopQt() {
  QObject::connect(&timer_, &QTimer::timeout, [this] { Run(); });
  timer_.start(10);
}

MessageLoopQt::~MessageLoopQt() {}

void MessageLoopQt::PostDelayedTask(Duration delay,
                                    Task task,
                                    const std::source_location& location) {
  assert(task);

  std::lock_guard<std::recursive_mutex> lock{mutex_};
  if (delay == Duration()) {
    immediate_queue_.emplace(std::move(task));
  } else {
    PendingTask pending_task{
        .task = std::move(task),
        .time = Clock::now() + delay,
        .sequence = sequence_num_++,
    };
    delayed_queue_.emplace(std::move(pending_task));
  }
}

size_t MessageLoopQt::GetTaskCount() const {
  std::lock_guard<std::recursive_mutex> lock{mutex_};
  return immediate_queue_.size() + delayed_queue_.size();
}

void MessageLoopQt::Run() {
  auto now = Clock::now();

  std::unique_lock<std::recursive_mutex> lock{mutex_};

  // Move due delayed tasks to the immediate queue.
  while (!delayed_queue_.empty() && delayed_queue_.top().time <= now) {
    immediate_queue_.emplace(
        std::move(const_cast<PendingTask&>(delayed_queue_.top()).task));
    delayed_queue_.pop();
  }

  // Run all immediate tasks.
  while (!immediate_queue_.empty()) {
    auto task = std::move(immediate_queue_.front());
    immediate_queue_.pop();
    lock.unlock();
    task();
    lock.lock();
  }
}
