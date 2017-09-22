#include "qt/message_loop_qt.h"

MessageLoopQt::MessageLoopQt()
    : sequence_num_(0),
      thread_task_runner_handle_(this) {
  timer_.start(50);
  QObject::connect(&timer_, &QTimer::timeout, [this] { Run(); });
}

MessageLoopQt::~MessageLoopQt() {
}

void MessageLoopQt::Run() {
  auto ticks = base::TimeTicks::Now();
  while (!queue_.empty() && queue_.top().delayed_run_time <= ticks) {
    auto pending_task = std::move(const_cast<base::PendingTask&>(queue_.top()));
    queue_.pop();
    std::move(pending_task.task).Run();
  }
}

bool MessageLoopQt::PostTaskHelper(const tracked_objects::Location& from_here,
                                   const base::Closure& task, base::TimeDelta delay,
                                   bool nestable) {
  assert(task);
  auto ticks = base::TimeTicks::Now() + delay;
  base::PendingTask pending_task(from_here, task, ticks, nestable);
  pending_task.sequence_num = sequence_num_++;
  queue_.emplace(std::move(pending_task));
  return true;
}
