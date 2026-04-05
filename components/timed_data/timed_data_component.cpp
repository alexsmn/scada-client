#include "components/timed_data/timed_data_component.h"

#include "components/timed_data/timed_data_controller.h"
#include "controller/controller_registry.h"

const WindowInfo kTimedDataWindowInfo = {
    ID_TIMED_DATA_VIEW,
    "TimeVal",
    u"Data",
    WIN_INS | WIN_DISALLOW_NEW | WIN_CAN_PRINT,
    0,
    0,
    0};

REGISTER_CONTROLLER(TimedDataController, kTimedDataWindowInfo);
