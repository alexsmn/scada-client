#pragma once

#include "aui/models/status_bar_model.h"
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
  void OnProgressStatus(const ProgressStatus& status);

  void UpdateUser();

  void NotifyPanelsChanged(int index);

  // NodeRefObserver
  virtual void OnNodeSemanticChanged(const scada::NodeId& node_id) override;

  base::ObserverList<aui::StatusBarModelObserver> observers_;

  Progress progress_{false};

  NodeRef user_node_;

  std::vector<boost::signals2::scoped_connection> connections_;

  static const int kSeverityPaneIndex = 2;
  static const int kUserPaneIndex = 3;
};
