#pragma once

namespace scada {
class ViewService;
}

class MainWindow;
class NodeRef;
class NodeService;
class WindowDefinition;

void ExecuteDefaultItemCommand(scada::ViewService& view_service,
                               NodeService& node_service,
                               const NodeRef& node,
                               MainWindow* main_window);

void OpenView(MainWindow* main_window, const WindowDefinition& def);
