#include "components/configuration_tree/nodes_view.h"
#include "controller_factory.h"

const WindowInfo kWindowInfo = {
    ID_NODES_VIEW, "Nodes", L"Узлы", WIN_SING | WIN_REQUIRES_ADMIN,
    200,           400,     0};

REGISTER_CONTROLLER(NodesView, kWindowInfo);
