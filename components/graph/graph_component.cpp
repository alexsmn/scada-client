#include "components/graph/graph_view.h"
#include "controller_factory.h"

const WindowInfo kWindowInfo = {
    ID_GRAPH_VIEW, "Graph", L"График", WIN_INS, 0, 0, IDR_GRAPH_POPUP};

REGISTER_CONTROLLER(GraphView, kWindowInfo);
