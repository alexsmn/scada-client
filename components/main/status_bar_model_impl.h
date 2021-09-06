#pragma once

#include "base/observer_list.h"
#include "controls/status_bar_model.h"
#include "services/progress_host.h"

namespace scada {
class SessionService;
}

class EventFetcher;
class NodeService;

struct StatusBarModelImplContext {
  scada::SessionService& session_service_;
  EventFetcher& event_fetcher_;
  NodeService& node_service_;
  ProgressHost& progress_host_;
};

class StatusBarModelImpl final : private StatusBarModelImplContext,
                                 public StatusBarModel {
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
  void OnProgressStatus(const ProgressStatus& status);

  base::ObserverList<StatusBarModelObserver> observers_;

  Progress progress_{false};

  boost::signals2::scoped_connection progress_host_connection_;
};
