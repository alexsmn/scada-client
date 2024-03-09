#pragma once

#include "node_service/node_observer.h"
#include "node_service/node_ref.h"

#include <boost/signals2/connection.hpp>

class Executor;
class NodeService;

class UserStatusProvider final
    : private NodeRefObserver,
      public std::enable_shared_from_this<UserStatusProvider> {
 public:
  using ChangeNotifier = std::function<void()>;

  UserStatusProvider(const std::shared_ptr<Executor>& executor,
                     NodeService& node_service,
                     scada::SessionService& session_service);

  ~UserStatusProvider();

  void Init(const ChangeNotifier& change_notifier);

  std::u16string GetText() const;

 private:
  void UpdateUser();

  // NodeRefObserver
  virtual void OnNodeSemanticChanged(const scada::NodeId& node_id) override;

  std::shared_ptr<Executor> executor_;
  NodeService& node_service_;
  scada::SessionService& session_service_;

  ChangeNotifier change_notifier_;
  NodeRef user_node_;
  boost::signals2::scoped_connection connection_;
};
