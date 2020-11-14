#pragma once

#include "base/callback_forward.h"
#include "base/memory/weak_ptr.h"
#include "base/observer_list.h"
#include "base/timer/timer.h"
#include "core/status.h"
#include "services/task_manager.h"

#include <functional>
#include <queue>
#include <set>

namespace scada {
class AttributeService;
class NodeManagementService;
}  // namespace scada

class LocalEvents;
class NodeService;
class Profile;
class ProgressDialog;

struct TaskManagerImplContext {
  NodeService& node_service_;
  scada::NodeManagementService& node_management_service_;
  scada::AttributeService& attribute_service_;
  LocalEvents& local_events_;
  Profile& profile_;
};

class TaskManagerImpl : private TaskManagerImplContext, public TaskManager {
 public:
  explicit TaskManagerImpl(TaskManagerImplContext&& context);
  ~TaskManagerImpl();

  // TaskManager
  virtual void PostInsertTask(const scada::NodeId& requested_id,
                              const scada::NodeId& parent_id,
                              const scada::NodeId& type_id,
                              scada::NodeAttributes attributes,
                              scada::NodeProperties properties,
                              InsertCallback callback) override;
  virtual void PostUpdateTask(const scada::NodeId& node_id,
                              scada::NodeAttributes attributes,
                              scada::NodeProperties properties,
                              UpdateCallback callback) override;
  virtual void PostDeleteTask(const scada::NodeId& node_id) override;
  virtual void PostAddReference(const scada::NodeId& reference_type_id,
                                const scada::NodeId& source_id,
                                const scada::NodeId& target_id) override;
  virtual void PostDeleteReference(const scada::NodeId& reference_type_id,
                                   const scada::NodeId& source_id,
                                   const scada::NodeId& target_id) override;
  virtual void AddObserver(TaskManagerObserver& observer) override;
  virtual void RemoveObserver(TaskManagerObserver& observer) override;

 private:
  using TaskMethod = std::function<void()>;

  struct Task {
    bool IsNull() const { return !task; }

    std::wstring title;
    std::function<void()> task;
  };

  void Run();
  void CancelProgressDialog();

  void PostTask(base::StringPiece16 title, TaskMethod task);
  void StartTask(Task&& task);

  void OnDeleteRecordComplete(scada::Status&& status,
                              std::vector<scada::NodeId>&& dependencies);

  void ReportRequestCompletion(const scada::Status& status,
                               const std::wstring& result_text);

  typedef std::queue<Task> TaskQueue;
  TaskQueue tasks_;

  int count = 0;  // initial task count
  DWORD start_time = 0;
  std::unique_ptr<ProgressDialog> progress_dialog_;

  base::RepeatingTimer timer_;

  Task running_task_;

  base::ObserverList<TaskManagerObserver> observers_;

  base::WeakPtrFactory<TaskManagerImpl> weak_factory_{this};
};
