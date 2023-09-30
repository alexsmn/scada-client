#include "components/hardware_tree/hardware_tree_component.h"

#include "components/hardware_tree/hardware_tree_view.h"
#include "controller/controller_registry.h"

const WindowInfo kHardwareTreeWindowInfo = {
    ID_HARDWARE_VIEW, "Subsystems", u"Оборудование", WIN_SING, 200, 400};

REGISTER_CONTROLLER(HardwareTreeView, kHardwareTreeWindowInfo);
