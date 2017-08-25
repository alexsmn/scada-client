#pragma once

class MainWindow;
class NodeRef;
class WindowDefinition;

void ExecuteDefaultItemCommand(MainWindow* main_window, const NodeRef& node);

void OpenView(MainWindow* main_window, const WindowDefinition& def);
