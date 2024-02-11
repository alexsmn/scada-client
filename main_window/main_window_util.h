#pragma once

#include "aui/key_codes.h"
#include "base/promise.h"
#include "filesystem/filesystem_commands.h"
#include "profile/window_definition.h"

class Executor;
class MainWindow;
class NodeRef;

scada::status_promise<void> OpenView(
    MainWindow* main_window,
    promise<WindowDefinition> window_def_promise,
    bool activate = true);

scada::status_promise<void> OpenView(MainWindow* main_window,
                                     const WindowDefinition& window_def,
                                     bool activate = true);

bool ExecuteDefaultNodeCommand(MainWindow* main_window,
                               const std::shared_ptr<Executor>& executor,
                               const OpenFileCommand& file_command,
                               const NodeRef& node,
                               aui::KeyModifiers key_modifiers);
