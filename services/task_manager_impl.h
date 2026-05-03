#pragma once

#include "base/any_executor.h"

#include "base/async_completion.h"
#include "base/awaitable.h"
#include "base/any_executor_timer.h"
#include "scada/coroutine_services.h"
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
  virtual Awaitable<void> PostTask(
      std::u16string_view description,
      const TaskLauncher& launcher) override;
  virtual Awaitable<scada::NodeId> PostInsertTask(
      const scada::NodeState& node_state) override;
  virtual Awaitable<void> PostUpdateTask(
      const scada::NodeId& node_id,
      scada::NodeAttributes attributes,
      scada::NodeProperties properties) override;
  virtual Awaitable<void> PostDeleteTask(
      const scada::NodeId& node_id) override;
  virtual Awaitable<void> PostAddReference(
      const scada::NodeId& reference_type_id,
      const scada::NodeId& source_id,
      const scada::NodeId& target_id) override;
  virtual Awaitable<void> PostDeleteReference(
      const scada::NodeId& reference_type_id,
      const scada::NodeId& source_id,
      const scada::NodeId& target_id) override;

 private:
  // A queued task's body. The coroutine runs to completion (or throws); the
  // returned Status drives the single local event written by
  // `ReportRequestCompletion`.
  using TaskMethod = std::function<Awaitable<scada::Status>()>;

  struct Task {
    bool IsNull() const { return !method; }

    std::u16string title;
    TaskMethod method;
    std::optional<base::AsyncCompletion> completion;
    std::function<void(const scada::Status&)> cancel;
  };

  void Run();
  void CancelProgress();

  Awaitable<void> PostTaskMethod(std::u16string_view title, TaskMethod method);

  template <class T>
  Awaitable<T> PostTypedTaskMethod(std::u16string_view title,
                                   std::function<Awaitable<T>()> method);

  [[nodiscard]] static Awaitable<scada::NodeId> RunInsertTask(
      std::shared_ptr<TaskManagerImpl> self,
      scada::NodeState node_state);
  [[nodiscard]] static Awaitable<scada::Status> RunUpdateTask(
      std::shared_ptr<TaskManagerImpl> self,
      scada::NodeId node_id,
      scada::NodeAttributes attributes,
      scada::NodeProperties properties);
  [[nodiscard]] static Awaitable<scada::Status> RunDeleteTask(
      std::shared_ptr<TaskManagerImpl> self,
      scada::NodeId node_id);
  [[nodiscard]] static Awaitable<scada::Status> RunAddReferenceTask(
      std::shared_ptr<TaskManagerImpl> self,
      scada::NodeId reference_type_id,
      scada::NodeId source_id,
      scada::NodeId target_id);
  [[nodiscard]] static Awaitable<scada::Status> RunDeleteReferenceTask(
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
