#include "components/modus/views/modus_controller.h"
#include "controller_factory.h"

const WindowInfo kWindowInfo = {ID_MODUS_VIEW,  "Modus", L"Схема", 0, 0, 0,
                                IDR_MODUS_POPUP};

REGISTER_CONTROLLER(ModusController, kWindowInfo);
