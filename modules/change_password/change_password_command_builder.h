#pragma once

#include "base/any_executor.h"
#include "controller/command_registry.h"
#include "core/selection_command_context.h"

#include <memory>

namespace scada {
class SessionService;
}

class LocalEvents;
class Profile;

struct ChangePasswordCommandBuilder {
  BasicCommand<SelectionCommandContext> Build();

  AnyExecutor executor_;
  LocalEvents& local_events_;
  Profile& profile_;
  scada::SessionService& session_service_;
};
