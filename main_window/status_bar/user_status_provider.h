#pragma once

#include "base/any_executor.h"

#include "node_service/node_ref.h"

#include <boost/signals2/connection.hpp>

class NodeService;

// Maps the session's two access-right tiers to a coarse role key (an English
// literal for Translate()): Configure ⇒ "Administrator", Control ⇒ "Operator",
// otherwise "Observer". Pure, for the status strip's user·role cell.
const char* UserRoleKey(bool can_configure, bool can_control);

class UserStatusProvider final
    : public std::enable_shared_from_this<UserStatusProvider> {
 public:
  using ChangeNotifier = std::function<void()>;

  UserStatusProvider(const AnyExecutor& executor,
                     NodeService& node_service,
                     scada::SessionService& session_service);

  ~UserStatusProvider();

  void Init(const ChangeNotifier& change_notifier);

  // The signed-in user with a coarse role suffix, e.g. "root · Administrator".
  std::u16string GetText() const;

 private:
  void UpdateUser();

  // Coarse role label derived from the session's access rights.
  std::u16string RoleLabel() const;

  AnyExecutor executor_;
  NodeService& node_service_;
  scada::SessionService& session_service_;

  ChangeNotifier change_notifier_;
  NodeRef user_node_;
  boost::signals2::scoped_connection connection_;
  boost::signals2::scoped_connection user_node_semantic_changed_connection_;
};
