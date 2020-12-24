#include "components/sheet/sheet_view.h"
#include "controller_factory.h"

// NOTE: Context menu depends on edit mode.
const WindowInfo kWindowInfo = {
    ID_SHEET_VIEW, "CusTable", L"Пользовательская таблица", WIN_INS, 0, 0,
    IDR_ITEM_POPUP};

REGISTER_CONTROLLER(SheetController, kWindowInfo);
