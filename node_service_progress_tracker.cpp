#include "node_service_progress_tracker.h"

#include "core/progress_host.h"
#include "node_service/node_service.h"

using namespace std::chrono_literals;

NodeServiceProgressTracker::NodeServiceProgressTracker(
    const std::shared_ptr<Executor> executor,
    NodeService& node_service,
    ProgressHost& progress_host)
    : node_service_{node_service},
      progress_host_{progress_host},
      update_timer_{std::move(executor)} {
  update_timer_.StartRepeating(300ms, [this] { OnTimer(); });
}

NodeServiceProgressTracker::~NodeServiceProgressTracker() {}

void NodeServiceProgressTracker::OnTimer() {
  const int pending_task_count = node_service_.GetPendingTaskCount();
  if (pending_task_count == pending_task_count_)
    return;

  pending_task_count_ = pending_task_count;
  max_pending_task_count_ =
      pending_task_count_ != 0
          ? std ::max(max_pending_task_count_, pending_task_count_)
          : 0;

  if (pending_task_count_ == max_pending_task_count_) {
    running_progress_.reset();

  } else {
    if (!running_progress_)
      running_progress_ = progress_host_.Start();

    running_progress_->SetProgress(
        max_pending_task_count_, max_pending_task_count_ - pending_task_count_);
  }
}
