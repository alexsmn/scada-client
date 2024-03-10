#pragma once

#include "controller/command_registry.h"
#include "core/selection_command_context.h"

namespace scada {
class SessionService;
}

class LocalEvents;
class Profile;

struct ChangePasswordCommandBuilder {
  BasicCommand<SelectionCommandContext> Build();

  LocalEvents& local_events_;
  Profile& profile_;
  scada::SessionService& session_service_;
};
