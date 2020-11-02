#pragma once

#include "base/observer_list.h"
#include "base/timer/timer.h"
#include "node_service/node_util.h"
#include "controls/status_bar_model.h"
#include "services/task_manager.h"

namespace scada {
class SessionService;
}

class EventFetcher;
class NodeService;

struct StatusBarModelImplContext {
  scada::SessionService& session_service_;
  EventFetcher& event_fetcher_;
  NodeService& node_service_;
  TaskManager& task_manager_;
};

class StatusBarModelImpl final : private StatusBarModelImplContext,
                                 public StatusBarModel,
                                 private TaskManagerObserver {
 public:
  explicit StatusBarModelImpl(StatusBarModelImplContext&& context);
  ~StatusBarModelImpl();

  // StatusBarModel
  virtual int GetPaneCount() override;
  virtual base::string16 GetPaneText(int index) override;
  virtual int GetPaneSize(int index) override;
  virtual Progress GetProgress() const override;
  virtual void AddObserver(StatusBarModelObserver& observer) override;
  virtual void RemoveObserver(StatusBarModelObserver& observer) override;

 private:
  void UpdatePanes();

  // TaskManagerObserver
  virtual void OnTaskManagerStatus(
      const TaskManagerObserver::Status& status) override;

  base::ObserverList<StatusBarModelObserver> observers_;

  base::RepeatingTimer update_timer_;

  Progress progress_{false};
};

inline StatusBarModelImpl::StatusBarModelImpl(
    StatusBarModelImplContext&& context)
    : StatusBarModelImplContext{std::move(context)} {
  task_manager_.AddObserver(*this);

  update_timer_.Start(
      FROM_HERE, base::TimeDelta::FromMilliseconds(500),
      base::Bind(&StatusBarModelImpl::UpdatePanes, base::Unretained(this)));
}

inline StatusBarModelImpl::~StatusBarModelImpl() {
  task_manager_.RemoveObserver(*this);
}

inline int StatusBarModelImpl::GetPaneCount() {
  return 6;
}

inline base::string16 StatusBarModelImpl::GetPaneText(int index) {
  switch (index) {
    case 1: {
      size_t unacked_event_count = event_fetcher_.unacked_events().size();
      return unacked_event_count
                 ? base::StringPrintf(L"Событий: %u", unacked_event_count)
                 : L"Нет событий";
    }

    case 2:
      return base::StringPrintf(L"Важность: %u", event_fetcher_.severity_min());

    case 3: {
      const auto& user_id = session_service_.GetUserId();
      return GetDisplayName(node_service_, user_id);
    }

    case 4: {
      base::TimeDelta ping_delay;
      auto connected = session_service_.IsConnected(&ping_delay);
      return connected ? L"Подключен" : L"Отключен";
    }

    case 5: {
      base::TimeDelta ping_delay;
      auto connected = session_service_.IsConnected(&ping_delay);
      return connected ? base::StringPrintf(
                             L"Отклик: %u мс",
                             static_cast<unsigned>(ping_delay.InMilliseconds()))
                       : L"Нет отклика";
    }

    default:
      return {};
  }
}

inline int StatusBarModelImpl::GetPaneSize(int index) {
  const int kSizes[] = {-1, 100, 100, 100, 100, 120};
  return kSizes[index];
}

StatusBarModel::Progress StatusBarModelImpl::GetProgress() const {
  return progress_;
}

inline void StatusBarModelImpl::AddObserver(StatusBarModelObserver& observer) {
  observers_.AddObserver(&observer);
}

inline void StatusBarModelImpl::RemoveObserver(
    StatusBarModelObserver& observer) {
  observers_.RemoveObserver(&observer);
}

inline void StatusBarModelImpl::UpdatePanes() {
  for (auto& o : observers_)
    o.OnPanesChanged(0, 6);
}

inline void StatusBarModelImpl::OnTaskManagerStatus(
    const TaskManagerObserver::Status& status) {
  progress_ = {status.active, status.range, status.current};
  for (auto& o : observers_)
    o.OnProgressChanged();
}
