#pragma once

#include "base/promise.h"
#include "window_definition.h"

class MainWindow;
class NodeRef;

void OpenView(MainWindow* main_window,
              promise<WindowDefinition> window_def_promise,
              bool activate = true);

void OpenView(MainWindow* main_window,
              const WindowDefinition& window_def,
              bool activate = true);

bool ExecuteDefaultNodeCommand(MainWindow* main_window,
                               const NodeRef& node,
                               unsigned shift);
