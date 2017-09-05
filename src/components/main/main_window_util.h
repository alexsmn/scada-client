#pragma once

class MainWindow;
class NodeRef;
class NodeRefService;
class WindowDefinition;

void ExecuteDefaultItemCommand(NodeRefService& node_service, const NodeRef& node, MainWindow* main_window);

void OpenView(MainWindow* main_window, const WindowDefinition& def);
