#pragma once

class MainWindow;
class NodeRef;
class NodeService;
class WindowDefinition;

void ExecuteDefaultItemCommand(NodeService& node_service, const NodeRef& node, MainWindow* main_window);

void OpenView(MainWindow* main_window, const WindowDefinition& def);
