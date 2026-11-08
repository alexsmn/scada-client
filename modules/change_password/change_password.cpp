#include "modules/change_password/change_password.h"

#include "aui/translation.h"
#include "base/awaitable.h"
#include "base/u16format.h"
#include "base/utf_convert.h"
#include "events/local_event_util.h"
#include "events/local_events.h"
#include "model/namespaces.h"
#include "model/security_node_ids.h"
#include "modules/change_password/change_password_dialog.h"
#include "node_service/node_ref.h"
#include "node_service/node_service.h"
#include "scada/co_result.h"
#include "scada/standard_node_ids.h"
#include "services/server_compatibility.h"

#include <optional>

namespace {

Awaitable<void> ReportPasswordChangeResultAsync(
    AnyExecutor executor,
    scada::CoStatus call,
    std::optional<scada::CoStatus> legacy_call,
    std::u16string title,
    bool root,
    LocalEvents& local_events,
    const Profile& profile) {
  auto status = co_await std::move(call);
  // A 2.5.7 Server has no UserManagement object and refuses the call as an
  // unknown node. It changes a password with the per-user ChangePassword
  // method instead, which always checks the account's current password
  // (2.5.7 source, scada-server-framework at b122fd7ad,
  // `security/security_io_manager.h`). So one's own password goes the 2.5
  // way; an administrator's reset, which has no current password to give,
  // cannot be done there at all and says so.
  if (IsMissingOnServer(status)) {
    if (!legacy_call) {
      local_events.ReportEvent(
          LocalEvents::SEV_ERROR,
          u16format(L"{} - {}", title,
                    Translate("A Server older than version 2.6 changes a "
                              "password only together with the current one: "
                              "the user changes it after signing in.")));
      co_return;
    }
    status = co_await std::move(*legacy_call);
  }
  if (status.code() == scada::StatusCode::Bad_NotWritable) {
    // The Server answers this for the built-in root account when its password
    // is configured on the server computer, and for the inter-process service
    // account `svc`, whose password is configured there always: every start
    // rewrites them, so a change made here would revert. The bare status name
    // would tell the operator nothing about where the password is changed
    // instead (backlog 882).
    local_events.ReportEvent(
        LocalEvents::SEV_ERROR,
        u16format(L"{} - {}", title,
                  root ? Translate("The root password is set on the server "
                                   "computer and cannot be changed from the "
                                   "Client. Change it there with: scada-setup "
                                   "set-password --root-password <new "
                                   "password>")
                       : Translate("The service account's password is set on "
                                   "the server computer and cannot be changed "
                                   "from the Client. Change it there with: "
                                   "scada-setup set-password --svc-password "
                                   "<new password>")));
    co_return;
  }
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
  // Built eagerly and lazy: it is sent only if the Server turns out to have
  // no UserManagement object (a 2.5.7 Server).
  std::optional<scada::CoStatus> legacy_call;
  if (context.self_service_) {
    legacy_call = context.user_.scada_node().call(
        scada::security::id::UserType_ChangePassword, current_password,
        new_password);
  }
  CoSpawn(context.executor_,
          [executor = context.executor_, call = std::move(call),
           legacy_call = std::move(legacy_call),
           title = u16format(Translate("Changing password for user {}"),
                             ToString16(context.user_.display_name())),
           root = ToString16(context.user_.display_name()) == u"root",
           &local_events = context.local_events_,
           &profile = context.profile_]() mutable -> Awaitable<void> {
            co_await ReportPasswordChangeResultAsync(
                std::move(executor), std::move(call), std::move(legacy_call),
                std::move(title), root, local_events, profile);
          });
}
