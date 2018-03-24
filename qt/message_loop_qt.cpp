#include "qt/message_loop_qt.h"

MessageLoopQt::MessageLoopQt() {
  QObject::connect(&timer_, &QTimer::timeout, [this] { Run(); });
  timer_.start(10);
}

MessageLoopQt::~MessageLoopQt() {}

void MessageLoopQt::Run() {
  auto ticks = base::TimeTicks::Now();

  std::unique_lock<std::recursive_mutex> lock{mutex_};
  while (!queue_.empty() && queue_.top().delayed_run_time <= ticks) {
    auto pending_task = std::move(const_cast<base::PendingTask&>(queue_.top()));
    queue_.pop();
    lock.unlock();
    std::move(pending_task.task).Run();
    lock.lock();
  }
}

bool MessageLoopQt::PostTaskHelper(const tracked_objects::Location& from_here,
                                   const base::Closure& task,
                                   base::TimeDelta delay,
                                   bool nestable) {
  assert(task);

  auto ticks = base::TimeTicks::Now() + delay;

  std::lock_guard<std::recursive_mutex> lock{mutex_};
  base::PendingTask pending_task(from_here, task, ticks, nestable);
  pending_task.sequence_num = sequence_num_++;
  queue_.emplace(std::move(pending_task));

  return true;
}
