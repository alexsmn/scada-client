#pragma once

#include "controller/command_registry.h"
#include "core/selection_command_context.h"

#include <memory>

namespace scada {
class SessionService;
}

class Executor;
class LocalEvents;
class Profile;

struct ChangePasswordCommandBuilder {
  BasicCommand<SelectionCommandContext> Build();

  std::shared_ptr<Executor> executor_;
  LocalEvents& local_events_;
  Profile& profile_;
  scada::SessionService& session_service_;
};
