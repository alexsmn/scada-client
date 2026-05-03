#pragma once

#include "base/any_executor.h"

#include "base/awaitable.h"
#include "filesystem/filesystem_commands.h"
#include "profile/window_definition.h"

class MainWindowInterface;
struct NodeCommandContext;

Awaitable<void> OpenView(MainWindowInterface* main_window,
                         const WindowDefinition& window_def,
                         bool activate = true);

bool ExecuteDefaultNodeCommand(const AnyExecutor& executor,
                               const OpenFileCommand& file_command,
                               const NodeCommandContext& context);
