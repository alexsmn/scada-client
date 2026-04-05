#include "components/table/table_component.h"

#include "components/table/table_view.h"
#include "controller/controller_registry.h"

const WindowInfo kTableWindowInfo = {
    ID_TABLE_VIEW, "Table", u"Table", WIN_INS | WIN_CAN_PRINT, 620, 400};

REGISTER_CONTROLLER(TableView, kTableWindowInfo);
