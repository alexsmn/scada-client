#include "accounts/account_status.h"

#include "aui/translation.h"
#include "base/u16format.h"
#include "events/local_event_util.h"
#include "events/local_events.h"
#include "model/namespaces.h"
#include "model/security_node_ids.h"
#include "node_service/node_service.h"
#include "node_service/node_util.h"
#include "scada/standard_node_ids.h"
#include "services/server_compatibility.h"

Awaitable<scada::Status> SetAccountDisabled(NodeService& node_service,
                                            std::u16string account,
                                            scada::UserConfiguration current,
                                            bool disabled) {
  const auto bits = static_cast<std::uint32_t>(current);
  const auto disabled_bit =
      static_cast<std::uint32_t>(scada::UserConfiguration::kDisabled);
  const scada::UInt32 mask =
      disabled ? (bits | disabled_bit) : (bits & ~disabled_bit);
  NodeRef user_management = node_service.GetNode(
      scada::NodeId{scada::id::Server_ServerConfiguration_UserManagement,
                    scada::NamespaceIndexes::NS0});
  co_return co_await user_management.scada_node().call(
      scada::NodeId{scada::id::UserManagement_ModifyUser,
                    scada::NamespaceIndexes::NS0},
      scada::LocalizedText{account}, /*modify_password=*/false,
      scada::LocalizedText{}, /*modify_user_configuration=*/true, mask,
      /*modify_description=*/false, scada::String{});
}

void ReportAccountChangeResult(const std::u16string& title,
                               const scada::Status& status,
                               LocalEvents& local_events,
                               const Profile& profile) {
  if (status.code() == scada::StatusCode::Bad_NotWritable) {
    local_events.ReportEvent(
        LocalEvents::SEV_ERROR,
        u16format(L"{} - {}", title,
                  Translate("This is a service account managed on the server "
                            "computer; it cannot be changed from the "
                            "Client.")));
    return;
  }
  // A 2.5.7 Server has neither the UserManagement object nor Roles, and
  // refuses both as unknown nodes. Naming the status ("Wrong node
  // identifier") would send the administrator looking for a broken account.
  if (IsMissingOnServer(status)) {
    local_events.ReportEvent(LocalEvents::SEV_ERROR,
                             u16format(L"{} - {}", title, ServerTooOldText()));
    return;
  }
  ReportRequestResult(title, status, local_events, profile);
}
