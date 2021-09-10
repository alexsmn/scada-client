#pragma once

#include "base/executor_timer.h"

#include <memory>

class Executor;
class NodeService;
class ProgressHost;
class RunningProgress;

class NodeServiceProgressTracker {
 public:
  NodeServiceProgressTracker(const std::shared_ptr<Executor> executor,
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

  ExecutorTimer update_timer_;
};
