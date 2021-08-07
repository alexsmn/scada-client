#include "components/hardware_tree/hardware_tree_component.h"

#include "components/hardware_tree/hardware_tree_view.h"
#include "controller_registry.h"

const WindowInfo kHardwareTreeWindowInfo = {
    ID_HARDWARE_VIEW, "Subsystems", L"Оборудование", WIN_SING, 200, 400};

REGISTER_CONTROLLER(HardwareTreeView, kHardwareTreeWindowInfo);
