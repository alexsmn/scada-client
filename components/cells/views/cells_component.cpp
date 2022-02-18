#include "components/cells/views/cell_view.h"
#include "controller_registry.h"

const WindowInfo kWindowInfo = {
    ID_CELLS_VIEW, "Cells", u"Ячейки", WIN_INS, 0, 0, 0};

REGISTER_CONTROLLER(CellView, kWindowInfo);
