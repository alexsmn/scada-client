#include "components/summary/summary_view.h"
#include "controller_factory.h"

const WindowInfo kWindowInfo = {ID_SUMMARY_VIEW, "Summ", L"Сводка",
                                WIN_INS | WIN_CAN_PRINT};

REGISTER_CONTROLLER(SummaryView, kWindowInfo);
