#pragma once

class MainWindow;
class NodeRef;
class WindowDefinition;

void OpenView(MainWindow* main_window,
              const WindowDefinition& def,
              bool activate = true);

bool ExecuteDefaultNodeCommand(MainWindow* main_window,
                               const NodeRef& node,
                               unsigned shift);
