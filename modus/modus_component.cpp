#include "modus/modus_component.h"

#include "controller/controller_registry.h"
#include "modus/modus_controller.h"

const WindowInfo kModusWindowInfo = {ID_MODUS_VIEW,  "Modus", u"Схема", 0, 0, 0,
                                     IDR_MODUS_POPUP};

REGISTER_CONTROLLER(ModusController, kModusWindowInfo);
