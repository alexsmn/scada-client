#pragma once

#include "common_resources.h"
#include "controller/window_info.h"

class ComponentApi;

static inline const WindowInfo kObjectTreeWindowInfo = {
    ID_OBJECT_VIEW, "Struct", u"Объекты", WIN_SING, 200, 400, 0};

void InitObjectTreeComponent(ComponentApi& api);
