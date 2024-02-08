#pragma once

#include "aui/models/status_bar_model.h"
#include "base/executor_timer.h"
#include "base/observer_list.h"
#include "node_service/node_observer.h"
#include "node_service/node_ref.h"
#include "services/progress_host.h"

namespace scada {
class SessionService;
}

class Executor;
class NodeEventProvider;
class NodeService;
class Profile;

struct StatusBarModelImplContext {
  const std::shared_ptr<Executor> executor_;
  scada::SessionService& session_service_;
  NodeEventProvider& node_event_provider_;
  NodeService& node_service_;
  ProgressHost& progress_host_;
  Profile& profile_;
};

class StatusBarModelImpl final : private StatusBarModelImplContext,
                                 public aui::StatusBarModel,
                                 private NodeRefObserver {
 public:
  explicit StatusBarModelImpl(StatusBarModelImplContext&& context);
  ~StatusBarModelImpl();

  // StatusBarModel
  virtual int GetPaneCount() const override;
  virtual std::u16string GetPaneText(int index) const override;
  virtual int GetPaneSize(int index) const override;
  virtual Progress GetProgress() const override;
  virtual void AddObserver(aui::StatusBarModelObserver& observer) override;
  virtual void RemoveObserver(aui::StatusBarModelObserver& observer) override;

 private:
  std::u16string GetEventPaneText() const;
  std::u16string GetSessionPaneText() const;
  std::u16string GetPingPaneText() const;

  void OnProgressStatus(const ProgressStatus& status);

  void UpdateUser();

  void NotifyPanelsChanged(int index, int count = 1);

  // NodeRefObserver
  virtual void OnNodeSemanticChanged(const scada::NodeId& node_id) override;

  base::ObserverList<aui::StatusBarModelObserver> observers_;

  Progress progress_{false};

  NodeRef user_node_;

  std::vector<boost::signals2::scoped_connection> connections_;

  ExecutorTimer session_poll_timer_;

  static const int kEventPaneIndex = 1;
  static const int kSeverityPaneIndex = 2;
  static const int kUserPaneIndex = 3;
  static const int kSessionFirstPaneIndex = 4;
  static const int kSessionPaneCount = 2;
  static const int kPaneCount = 6;
};
