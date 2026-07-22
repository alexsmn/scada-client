#include "main_window/status_bar/user_status_provider.h"

#include "aui/translation.h"
#include "base/any_executor_dispatch.h"
#include "base/check.h"
#include "events/node_event_provider.h"
#include "node_service/node_service.h"
#include "node_service/node_util.h"
#include "profile/profile.h"
#include "scada/privileges.h"
#include "scada/session_service.h"

const char* UserRoleKey(bool can_configure, bool can_control) {
  if (can_configure)
    return "Administrator";
  if (can_control)
    return "Operator";
  return "Observer";
}

UserStatusProvider::UserStatusProvider(const AnyExecutor& executor,
                                       NodeService& node_service,
                                       scada::SessionService& session_service)
    : executor_{executor},
      node_service_{node_service},
      session_service_{session_service} {}

UserStatusProvider::~UserStatusProvider() = default;

void UserStatusProvider::Init(const ChangeNotifier& change_notifier) {
  scada::base::Check(!weak_from_this().expired());

  change_notifier_ = change_notifier;

  // TODO: weak_ptr.
  connection_ = session_service_.SubscribeSessionStateChanged(BindExecutor(
      executor_,
      [this, ref = shared_from_this()](
          bool connected, const scada::Status& status) { UpdateUser(); }));

  UpdateUser();
}

std::u16string UserStatusProvider::GetText() const {
  const std::u16string user = ToString16(user_node_.display_name());
  const std::u16string role = RoleLabel();
  return user.empty() ? role : user + u" · " + role;
}

std::u16string UserStatusProvider::RoleLabel() const {
  // The session exposes privileges, not a role name; UserRoleKey maps them.
  return Translate(
      UserRoleKey(session_service_.HasPrivilege(scada::Privilege::Configure),
                  session_service_.HasPrivilege(scada::Privilege::Control)));
}

void UserStatusProvider::UpdateUser() {
  auto user_id = session_service_.GetUserId();
  if (user_id == user_node_.node_id()) {
    return;
  }

  user_node_ = node_service_.GetNode(user_id);
  user_node_.StartFetch(NodeFetchStatus::NodeOnly);
  user_node_semantic_changed_connection_ =
      user_node_.SubscribeNodeSemanticChanged(
          [this](const scada::NodeId&) { change_notifier_(); });

  change_notifier_();
}
