#include "components/node_properties/node_property_component.h"

#include "components/node_properties/node_property_controller.h"
#include "controller_registry.h"

const WindowInfo kNodePropertyWindowInfo = {
    ID_NEW_PROPERTY_VIEW,
    "NewProps",
    L"Параметры",
    WIN_DISALLOW_NEW | WIN_REQUIRES_ADMIN,
    200,
    400};

REGISTER_CONTROLLER(NodePropertyController, kNodePropertyWindowInfo);
