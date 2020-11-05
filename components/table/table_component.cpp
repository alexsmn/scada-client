#include "components/table/table_view.h"
#include "controller_factory.h"

const WindowInfo kWindowInfo = {ID_TABLE_VIEW,           "Table", L"Таблица",
                                WIN_INS | WIN_CAN_PRINT, 620,     400};

REGISTER_CONTROLLER(TableView, kWindowInfo);
