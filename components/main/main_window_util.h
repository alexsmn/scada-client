#pragma once

#include "base/promise.h"
#include "aui/key_codes.h"
#include "profile/window_definition.h"

class Executor;
class FileRegistry;
class MainWindow;
class NodeRef;

void OpenView(MainWindow* main_window,
              promise<WindowDefinition> window_def_promise,
              bool activate = true);

void OpenView(MainWindow* main_window,
              const WindowDefinition& window_def,
              bool activate = true);

bool ExecuteDefaultNodeCommand(MainWindow* main_window,
                               const std::shared_ptr<Executor>& executor,
                               const FileRegistry& file_registry,
                               const NodeRef& node,
                               aui::KeyModifiers key_modifiers);
