#pragma once

namespace scada {
class ViewService;
}

class MainWindow;
class NodeRef;
class NodeService;
class WindowDefinition;

void OpenView(MainWindow* main_window, const WindowDefinition& def);

void ExecuteDefaultNodeCommand(scada::ViewService& view_service,
                               NodeService& node_service,
                               const NodeRef& node,
                               MainWindow* main_window);
