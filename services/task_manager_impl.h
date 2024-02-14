#pragma once

#include "base/executor_timer.h"
#include "scada/status.h"
#include "services/task_manager.h"

#include <functional>
#include <queue>

namespace scada {
class AttributeService;
class NodeManagementService;
}  // namespace scada

class Executor;
class LocalEvents;
class NodeService;
class Profile;
class ProgressHost;
class RunningProgress;

struct TaskManagerImplContext {
  const std::shared_ptr<Executor> executor_;
  NodeService& node_service_;
  scada::AttributeService& attribute_service_;
  scada::NodeManagementService& node_management_service_;
  LocalEvents& local_events_;
  Profile& profile_;
  ProgressHost& progress_host_;
};

class TaskManagerImpl : private TaskManagerImplContext,
                        public TaskManager,
                        public std::enable_shared_from_this<TaskManagerImpl> {
 public:
  explicit TaskManagerImpl(TaskManagerImplContext&& context);
  ~TaskManagerImpl();

  // TaskManager
  virtual scada::status_promise<void> PostTask(
      std::u16string_view description,
      const TaskLauncher& launcher) override;
  virtual promise<scada::NodeId> PostInsertTask(
      const scada::NodeId& requested_id,
      const scada::NodeId& parent_id,
      const scada::NodeId& type_id,
      scada::NodeAttributes attributes,
      scada::NodeProperties properties,
      std::vector<scada::ReferenceDescription> references) override;
  virtual scada::status_promise<void> PostUpdateTask(
      const scada::NodeId& node_id,
      scada::NodeAttributes attributes,
      scada::NodeProperties properties) override;
  virtual scada::status_promise<void> PostDeleteTask(
      const scada::NodeId& node_id) override;
  virtual scada::status_promise<void> PostAddReference(
      const scada::NodeId& reference_type_id,
      const scada::NodeId& source_id,
      const scada::NodeId& target_id) override;
  virtual scada::status_promise<void> PostDeleteReference(
      const scada::NodeId& reference_type_id,
      const scada::NodeId& source_id,
      const scada::NodeId& target_id) override;

 private:
  using TaskMethod = std::function<void()>;

  struct Task {
    bool IsNull() const { return !task; }

    std::u16string title;
    std::function<void()> task;
    scada::status_promise<void> promise;
  };

  void Run();
  void CancelProgress();

  scada::status_promise<void> PostTask(std::u16string_view title,
                                       TaskMethod task);
  void StartTask(Task&& task);

  void ReportRequestCompletion(const scada::Status& status,
                               const std::u16string& result_text);

  typedef std::queue<Task> TaskQueue;
  TaskQueue tasks_;

  int count_ = 0;  // initial task count
  std::optional<std::chrono::steady_clock::time_point> start_time_;
  std::unique_ptr<RunningProgress> running_progress_;

  ExecutorTimer timer_{executor_};

  Task running_task_;
};
