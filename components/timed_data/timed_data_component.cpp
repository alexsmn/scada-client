#include "components/timed_data/timed_data_view.h"
#include "controller_factory.h"

const WindowInfo kWindowInfo = {ID_TIMED_DATA_VIEW,
                                "TimeVal",
                                L"Данные",
                                WIN_INS | WIN_DISALLOW_NEW | WIN_CAN_PRINT,
                                0,
                                0,
                                0};

REGISTER_CONTROLLER(TimedDataView, kWindowInfo);
