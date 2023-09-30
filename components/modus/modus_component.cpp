#include "components/modus/modus_component.h"

#include "components/modus/modus_controller.h"
#include "controller/controller_registry.h"

const WindowInfo kModusWindowInfo = {ID_MODUS_VIEW,  "Modus", u"Схема", 0, 0, 0,
                                     IDR_MODUS_POPUP};

REGISTER_CONTROLLER(ModusController, kModusWindowInfo);
