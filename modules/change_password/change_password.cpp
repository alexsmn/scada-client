#include "modules/change_password/change_password.h"

#include "base/awaitable.h"
#include "base/u16format.h"
#include "events/local_event_util.h"
#include "base/utf_convert.h"
#include "model/namespaces.h"
#include "model/security_node_ids.h"
#include "node_service/node_service.h"
#include "scada/standard_node_ids.h"
#include "modules/change_password/change_password_dialog.h"
#include "node_service/node_ref.h"
#include "scada/co_result.h"

namespace {

Awaitable<void> ReportPasswordChangeResultAsync(AnyExecutor executor,
                                                scada::CoStatus call,
                                                std::u16string title,
                                                LocalEvents& local_events,
                                                const Profile& profile) {
  auto status = co_await std::move(call);
  ReportRequestResult(title, status, local_events, profile);
  co_return;
}

}  // namespace

void ChangePassword(const ChangePasswordContext& context,
                    const scada::LocalizedText& current_password,
                    const scada::LocalizedText& new_password) {
  // Both methods live on the standard UserManagement object (OPC UA Part 18
  // §5.2), not on the per-user node: the standard user model keys accounts by
  // NAME and has no node to call a method on.
  const scada::NodeId user_management{
      scada::id::Server_ServerConfiguration_UserManagement,
      scada::NamespaceIndexes::NS0};
  NodeRef target = context.node_service_
                       ? context.node_service_->GetNode(user_management)
                       : NodeRef{};

  // §5.2.8 for one's own account (proves the old password), §5.2.7 for an
  // administrator resetting someone else's (does not require it). Sending the
  // old password on the reset path would be meaningless — the server does not
  // check it there.
  auto call =
      context.self_service_
          ? target.scada_node().call(scada::id::UserManagement_ChangePassword,
                                     current_password, new_password)
          : target.scada_node().call(
                scada::id::UserManagement_ModifyUser,
                context.user_.display_name(), /*modify_password=*/true,
                new_password, /*modify_user_configuration=*/false,
                static_cast<scada::UInt32>(0), /*modify_description=*/false,
                scada::String{});
  CoSpawn(context.executor_,
          [executor = context.executor_, call = std::move(call),
           title = u16format(L"Changing password for user {}",
                             ToString16(context.user_.display_name())),
           &local_events = context.local_events_,
           &profile = context.profile_]() mutable -> Awaitable<void> {
            co_await ReportPasswordChangeResultAsync(
                std::move(executor), std::move(call), std::move(title),
                local_events, profile);
          });
}
