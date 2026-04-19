#pragma once

#include "base/awaitable.h"
#include "base/executor_timer.h"
#include "scada/coroutine_services.h"
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

  // For testing.
  bool IsRunning() const;

  // TaskManager
  virtual promise<void> PostTask(
      std::u16string_view description,
      const TaskLauncher& launcher) override;
  virtual promise<scada::NodeId> PostInsertTask(
      const scada::NodeState& node_state) override;
  virtual promise<void> PostUpdateTask(
      const scada::NodeId& node_id,
      scada::NodeAttributes attributes,
      scada::NodeProperties properties) override;
  virtual promise<void> PostDeleteTask(
      const scada::NodeId& node_id) override;
  virtual promise<void> PostAddReference(
      const scada::NodeId& reference_type_id,
      const scada::NodeId& source_id,
      const scada::NodeId& target_id) override;
  virtual promise<void> PostDeleteReference(
      const scada::NodeId& reference_type_id,
      const scada::NodeId& source_id,
      const scada::NodeId& target_id) override;

 private:
  // A queued task's body. The coroutine runs to completion (or throws); the
  // returned Status drives the single local event written by
  // `ReportRequestCompletion`. Callers that need to hand back a typed value
  // (e.g. an inserted node id) resolve their own `promise<T>` from inside the
  // coroutine before `co_return`ing a success status.
  using TaskMethod = std::function<Awaitable<scada::Status>()>;

  struct Task {
    bool IsNull() const { return !method; }

    std::u16string title;
    TaskMethod method;
    promise<void> promise;
  };

  void Run();
  void CancelProgress();

  promise<void> PostTaskMethod(std::u16string_view title, TaskMethod method);
  void StartTask(Task&& task);
  Awaitable<void> RunTaskBody(TaskMethod method);

  void ReportRequestCompletion(const scada::Status& status,
                               const std::u16string& result_text);

  using TaskQueue = std::queue<Task>;
  TaskQueue tasks_;

  int count_ = 0;  // initial task count
  std::optional<std::chrono::steady_clock::time_point> start_time_;
  std::unique_ptr<RunningProgress> running_progress_;

  ExecutorTimer timer_{executor_};

  Task running_task_;

  // Coroutine adapters over the callback-based core services supplied via
  // `TaskManagerImplContext`. Built once at construction so task coroutines can
  // `co_await` them directly instead of chaining `.then()` on the legacy
  // promise wrappers.
  scada::CallbackToCoroutineAttributeServiceAdapter co_attribute_service_{
      executor_, attribute_service_};
  scada::CallbackToCoroutineNodeManagementServiceAdapter
      co_node_management_service_{executor_, node_management_service_};
};
