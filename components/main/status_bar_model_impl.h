#pragma once

#include "base/observer_list.h"
#include "base/timer/timer.h"
#include "controls/status_bar_model.h"
#include "services/task_manager.h"

#include <functional>

namespace scada {
class SessionService;
}

class EventFetcher;
class NodeService;

using PendingTaskProvider = std::function<int()>;

struct StatusBarModelImplContext {
  scada::SessionService& session_service_;
  EventFetcher& event_fetcher_;
  NodeService& node_service_;
  TaskManager& task_manager_;
  const PendingTaskProvider pending_task_provider_;
};

class StatusBarModelImpl final : private StatusBarModelImplContext,
                                 public StatusBarModel,
                                 private TaskManagerObserver {
 public:
  explicit StatusBarModelImpl(StatusBarModelImplContext&& context);
  ~StatusBarModelImpl();

  // StatusBarModel
  virtual int GetPaneCount() override;
  virtual std::wstring GetPaneText(int index) override;
  virtual int GetPaneSize(int index) override;
  virtual Progress GetProgress() const override;
  virtual void AddObserver(StatusBarModelObserver& observer) override;
  virtual void RemoveObserver(StatusBarModelObserver& observer) override;

 private:
  void OnTimer();

  void UpdateProgress();

  // TaskManagerObserver
  virtual void OnTaskManagerStatus(
      const TaskManagerObserver::Status& status) override;

  base::ObserverList<StatusBarModelObserver> observers_;

  base::RepeatingTimer update_timer_;

  int max_pending_task_count_ = 0;
  int pending_task_count_ = 0;

  TaskManagerObserver::Status task_manager_status_;

  Progress progress_{false};
};
