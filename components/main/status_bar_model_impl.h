#pragma once

#include "base/observer_list.h"
#include "base/timer/timer.h"
#include "common/node_util.h"
#include "controls/status_bar_model.h"

namespace events {
class EventManager;
}

namespace scada {
class SessionService;
}

class NodeService;

struct StatusBarModelImplContext {
  scada::SessionService& session_service_;
  events::EventManager& event_manager_;
  NodeService& node_service_;
};

class StatusBarModelImpl final : private StatusBarModelImplContext,
                                 public StatusBarModel {
 public:
  explicit StatusBarModelImpl(StatusBarModelImplContext&& context);

  virtual int GetPaneCount() override;
  virtual base::string16 GetPaneText(int index) override;
  virtual int GetPaneSize(int index) override;

  virtual void AddObserver(StatusBarModelObserver& observer) override;
  virtual void RemoveObserver(StatusBarModelObserver& observer) override;

 private:
  void Update();

  base::ObserverList<StatusBarModelObserver> observers_;

  base::RepeatingTimer update_timer_;
};

inline StatusBarModelImpl::StatusBarModelImpl(
    StatusBarModelImplContext&& context)
    : StatusBarModelImplContext{std::move(context)} {
  update_timer_.Start(
      FROM_HERE, base::TimeDelta::FromMilliseconds(500),
      base::Bind(&StatusBarModelImpl::Update, base::Unretained(this)));
}

inline int StatusBarModelImpl::GetPaneCount() {
  return 6;
}

inline base::string16 StatusBarModelImpl::GetPaneText(int index) {
  switch (index) {
    case 1: {
      size_t unacked_event_count = event_manager_.unacked_events().size();
      return unacked_event_count
                 ? base::StringPrintf(L"Событий: %u", unacked_event_count)
                 : L"Нет событий";
    }

    case 2:
      return base::StringPrintf(L"Важность: %u", event_manager_.severity_min());

    case 3: {
      auto& user_id = session_service_.GetUserId();
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

inline void StatusBarModelImpl::AddObserver(StatusBarModelObserver& observer) {
  observers_.AddObserver(&observer);
}

inline void StatusBarModelImpl::RemoveObserver(
    StatusBarModelObserver& observer) {
  observers_.RemoveObserver(&observer);
}

inline void StatusBarModelImpl::Update() {
  for (auto& o : observers_)
    o.OnPanesChanged(0, 6);
}
