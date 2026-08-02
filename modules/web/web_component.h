#pragma once

#include "base/any_executor.h"
#include "controller/window_info.h"
#include "resources/common_resources.h"

template <class T>
class BasicCommandRegistry;
class UiCommandRegistry;
struct GlobalCommandContext;

// Used directly by Modus controller.
inline const WindowInfo kWebWindowInfo = {ID_WEB_VIEW, "Web", u"Web"};

void RegisterWebCommands(
    AnyExecutor executor,
    BasicCommandRegistry<GlobalCommandContext>& global_commands,
    UiCommandRegistry& ui_command_registry);
