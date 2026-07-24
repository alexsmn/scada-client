#pragma once

#include "base/any_executor.h"

#include "base/any_executor_timer.h"
#include "base/async_completion.h"
#include "base/awaitable.h"
#include "scada/attribute_service.h"
#include "scada/co_result.h"
#include "scada/node_management_service.h"
#include "scada/status.h"
#include "services/task_manager.h"

#include <functional>
#include <optional>
#include <queue>

namespace scada {
class AttributeService;
class NodeManagementService;
}  // namespace scada

class LocalEvents;
class NodeService;
class Profile;
class ProgressHost;
class RunningProgress;

struct TaskManagerImplContext {
  const AnyExecutor executor_;
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
  virtual scada::CoStatus PostTask(std::u16string_view description,
                                   const TaskLauncher& launcher) override;
  virtual scada::CoStatusOr<scada::NodeId> PostInsertTask(
      const scada::NodeState& node_state) override;
  virtual scada::CoStatus PostUpdateTask(
      const scada::NodeId& node_id,
      scada::NodeAttributes attributes,
      scada::NodeProperties properties) override;
  virtual scada::CoStatus PostDeleteTask(const scada::NodeId& node_id) override;
  virtual scada::CoStatus PostAddReference(
      const scada::NodeId& reference_type_id,
      const scada::NodeId& source_id,
      const scada::NodeId& target_id) override;
  virtual scada::CoStatus PostDeleteReference(
      const scada::NodeId& reference_type_id,
      const scada::NodeId& source_id,
      const scada::NodeId& target_id) override;

 private:
  // A queued task's body. The coroutine runs to completion (or throws); the
  // returned Status drives the single local event written by
  // `ReportRequestCompletion`.
  using TaskMethod = std::function<scada::CoStatus()>;

  struct Task {
    bool IsNull() const { return !method; }

    std::u16string title;
    TaskMethod method;
    std::optional<scada::base::AsyncCompletion> completion;
    std::function<void(const scada::Status&)> cancel;
  };

  void Run();
  void CancelProgress();

  // Enqueues the task before returning, so the caller may discard the
  // returned awaitable for fire-and-forget usage (see the `TaskManager`
  // interface contract). Deliberately NOT a coroutine: a lazy coroutine here
  // regressed every call site that discarded the result — the task was never
  // queued. Only the returned result waiter is lazy.
  scada::CoStatus PostTaskMethod(std::u16string title, TaskMethod method);

  template <class T>
  scada::CoStatusOr<T> PostTypedTaskMethod(
      std::u16string title,
      std::function<scada::CoStatusOr<T>()> method);

  [[nodiscard]] static scada::CoStatusOr<scada::NodeId> RunInsertTask(
      std::shared_ptr<TaskManagerImpl> self,
      scada::NodeState node_state);
  [[nodiscard]] static scada::CoStatus RunUpdateTask(
      std::shared_ptr<TaskManagerImpl> self,
      scada::NodeId node_id,
      scada::NodeAttributes attributes,
      scada::NodeProperties properties);
  [[nodiscard]] static scada::CoStatus RunDeleteTask(
      std::shared_ptr<TaskManagerImpl> self,
      scada::NodeId node_id);
  [[nodiscard]] static scada::CoStatus RunAddReferenceTask(
      std::shared_ptr<TaskManagerImpl> self,
      scada::NodeId reference_type_id,
      scada::NodeId source_id,
      scada::NodeId target_id);
  [[nodiscard]] static scada::CoStatus RunDeleteReferenceTask(
      std::shared_ptr<TaskManagerImpl> self,
      scada::NodeId reference_type_id,
      scada::NodeId source_id,
      scada::NodeId target_id);

  void StartTask(Task&& task);
  Awaitable<void> RunTaskBody(TaskMethod method);

  void ReportRequestCompletion(const scada::Status& status,
                               const std::u16string& result_text);

  using TaskQueue = std::queue<Task>;
  TaskQueue tasks_;

  int count_ = 0;  // initial task count
  std::optional<std::chrono::steady_clock::time_point> start_time_;
  std::unique_ptr<RunningProgress> running_progress_;

  AnyExecutorTimer timer_{executor_};

  Task running_task_;
};
