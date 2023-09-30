#include "components/graph/graph_component.h"

#include "components/graph/graph_view.h"
#include "controller/controller_registry.h"

const WindowInfo kGraphWindowInfo = {
    ID_GRAPH_VIEW, "Graph", u"График", WIN_INS, 0, 0, IDR_GRAPH_POPUP};

REGISTER_CONTROLLER(GraphView, kGraphWindowInfo);
