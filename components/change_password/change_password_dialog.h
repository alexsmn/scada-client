#pragma once

#include "components/change_password/change_password_dialog.h"
#include "node_service/node_ref.h"

class DialogService;
class LocalEvents;
class Profile;

struct ChangePasswordContext {
  const NodeRef user_;
  LocalEvents& local_events_;
  Profile& profile_;
};

void ShowChangePasswordDialog(DialogService& dialog_service,
                              ChangePasswordContext&& context);
