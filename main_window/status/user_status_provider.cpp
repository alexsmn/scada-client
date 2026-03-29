#include "main_window/status/user_status_provider.h"

#include "base/executor.h"
#include "events/node_event_provider.h"
#include "node_service/node_service.h"
#include "node_service/node_util.h"
#include "profile/profile.h"
#include "scada/session_service.h"

UserStatusProvider::UserStatusProvider(
    const std::shared_ptr<Executor>& executor,
    NodeService& node_service,
    scada::SessionService& session_service)
    : executor_{executor},
      node_service_{node_service},
      session_service_{session_service} {}

UserStatusProvider::~UserStatusProvider() {
  user_node_.Unsubscribe(*this);
}

void UserStatusProvider::Init(const ChangeNotifier& change_notifier) {
  assert(shared_from_this());

  change_notifier_ = change_notifier;

  // TODO: weak_ptr.
  connection_ = session_service_.SubscribeSessionStateChanged(BindExecutor(
      executor_,
      [this, ref = shared_from_this()](bool connected, const scada::Status& status) {
        UpdateUser();
      }));

  UpdateUser();
}

std::u16string UserStatusProvider::GetText() const {
  return user_node_.display_name();
}

void UserStatusProvider::UpdateUser() {
  auto user_id = session_service_.GetUserId();
  if (user_id == user_node_.node_id()) {
    return;
  }

  user_node_.Unsubscribe(*this);

  user_node_ = node_service_.GetNode(user_id);
  user_node_.Fetch(NodeFetchStatus::NodeOnly());
  user_node_.Subscribe(*this);

  change_notifier_();
}

void UserStatusProvider::OnNodeSemanticChanged(const scada::NodeId& node_id) {
  change_notifier_();
}
