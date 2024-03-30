#pragma once

#include "base/promise.h"
#include "filesystem/filesystem_commands.h"
#include "profile/window_definition.h"

class Executor;
class MainWindowInterface;
struct NodeCommandContext;

scada::status_promise<void> OpenView(
    MainWindowInterface* main_window,
    promise<WindowDefinition> window_def_promise,
    bool activate = true);

scada::status_promise<void> OpenView(MainWindowInterface* main_window,
                                     const WindowDefinition& window_def,
                                     bool activate = true);

bool ExecuteDefaultNodeCommand(const std::shared_ptr<Executor>& executor,
                               const OpenFileCommand& file_command,
                               const NodeCommandContext& context);
