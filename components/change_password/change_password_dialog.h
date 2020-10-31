#pragma once

#include "node_service/node_ref.h"
#include "components/change_password/change_password_dialog.h"

namespace scada {
class NodeManagementService;
}  // namespace scada

class DialogService;
class LocalEvents;
class Profile;

struct ChangePasswordContext {
  const NodeRef user_;
  scada::NodeManagementService& node_management_service_;
  LocalEvents& local_events_;
  Profile& profile_;
};

void ShowChangePasswordDialog(DialogService& dialog_service,
                              ChangePasswordContext&& context);
