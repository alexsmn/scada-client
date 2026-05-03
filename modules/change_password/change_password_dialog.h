#pragma once

#include "base/any_executor.h"

#include "node_service/node_ref.h"

#include <memory>

class DialogService;
class LocalEvents;
class Profile;

struct ChangePasswordContext {
  const NodeRef user_;
  AnyExecutor executor_;
  LocalEvents& local_events_;
  Profile& profile_;
};

void ShowChangePasswordDialog(DialogService& dialog_service,
                              ChangePasswordContext&& context);
