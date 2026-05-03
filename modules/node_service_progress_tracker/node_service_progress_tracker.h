#pragma once

#include "base/any_executor.h"

#include "base/any_executor_timer.h"

#include <memory>

class NodeService;
class ProgressHost;
class RunningProgress;

class NodeServiceProgressTracker {
 public:
  NodeServiceProgressTracker(const AnyExecutor executor,
                             NodeService& node_service,
                             ProgressHost& progress_host);
  ~NodeServiceProgressTracker();

 private:
  void OnTimer();

  NodeService& node_service_;
  ProgressHost& progress_host_;

  int max_pending_task_count_ = 0;
  int pending_task_count_ = 0;

  std::unique_ptr<RunningProgress> running_progress_;

  AnyExecutorTimer update_timer_;
};
