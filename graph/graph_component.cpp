#include "graph/graph_component.h"

#include "controller/controller_registry.h"
#include "graph/graph_view.h"

const WindowInfo kGraphWindowInfo = {
    ID_GRAPH_VIEW, "Graph", u"Graph", WIN_INS, 0, 0, IDR_GRAPH_POPUP};

REGISTER_CONTROLLER(GraphView, kGraphWindowInfo);
