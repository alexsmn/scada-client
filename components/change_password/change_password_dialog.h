#pragma once

#include "node_service/node_ref.h"

#include <memory>

class DialogService;
class Executor;
class LocalEvents;
class Profile;

struct ChangePasswordContext {
  const NodeRef user_;
  std::shared_ptr<Executor> executor_;
  LocalEvents& local_events_;
  Profile& profile_;
};

void ShowChangePasswordDialog(DialogService& dialog_service,
                              ChangePasswordContext&& context);
