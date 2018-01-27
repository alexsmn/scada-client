#pragma once

#include "base/callback_forward.h"
#include "base/memory/weak_ptr.h"
#include "base/timer/timer.h"
#include "services/task_manager.h"

#include <functional>
#include <queue>
#include <set>

namespace scada {
class NodeManagementService;
}

class LocalEvents;
class NodeRefService;
class Profile;
class ProgressDialog;

struct TaskManagerImplContext {
  NodeRefService& node_service_;
  scada::NodeManagementService& node_management_service_;
  LocalEvents& local_events_;
  Profile& profile_;
};

class TaskManagerImpl : private TaskManagerImplContext, public TaskManager {
 public:
  explicit TaskManagerImpl(TaskManagerImplContext&& context);
  ~TaskManagerImpl();

  using TaskMethod = std::function<void()>;
  using InsertCallback =
      std::function<void(scada::Status status, const scada::NodeId& node_id)>;
  using UpdateCallback = std::function<void(scada::Status status)>;

  void PostInsertTask(const scada::NodeId& requested_id,
                      const scada::NodeId& parent_id,
                      const scada::NodeId& type_id,
                      scada::NodeAttributes attributes,
                      scada::NodeProperties properties,
                      InsertCallback callback = {});
  void PostUpdateTask(const scada::NodeId& node_id,
                      scada::NodeAttributes attributes,
                      scada::NodeProperties properties,
                      UpdateCallback callback = {});
  void PostDeleteTask(const scada::NodeId& node_id);
  void PostAddReference(const scada::NodeId& reference_type_id,
                        const scada::NodeId& source_id,
                        const scada::NodeId& target_id);
  void PostDeleteReference(const scada::NodeId& reference_type_id,
                           const scada::NodeId& source_id,
                           const scada::NodeId& target_id);

 private:
  struct Task {
    bool IsNull() const { return !task; }

    base::string16 title;
    std::function<void()> task;
  };

  void Run();
  void CancelProgressDialog();

  void PostTask(base::StringPiece16 title, TaskMethod task);
  void StartTask(Task&& task);

  void OnDeleteRecordComplete(const scada::Status& status,
                              const std::set<scada::NodeId>* relations);

  void ReportRequestCompletion(const scada::Status& status,
                               const base::string16& result_text);

  typedef std::queue<Task> TaskQueue;
  TaskQueue tasks_;
  unsigned next_task_id_ = 1;

  int count = 0;  // initial task count
  DWORD start_time = 0;
  std::unique_ptr<ProgressDialog> progress_dialog_;

  base::RepeatingTimer timer_;

  Task running_task_;

  base::WeakPtrFactory<TaskManagerImpl> weak_factory_{this};
};
