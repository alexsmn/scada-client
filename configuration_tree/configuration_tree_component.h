#pragma once

#include "common_resources.h"
#include "controller/window_info.h"

static inline const WindowInfo kObjectTreeWindowInfo = {
    ID_OBJECT_VIEW, "Struct", u"Объекты", WIN_SING, 200, 400, 0};

inline static const WindowInfo kHardwareTreeWindowInfo = {
    ID_HARDWARE_VIEW, "Subsystems", u"Оборудование", WIN_SING, 200, 400};

inline static const WindowInfo kNodesWindowInfo = {
    ID_NODES_VIEW, "Nodes", u"Узлы", WIN_SING | WIN_REQUIRES_ADMIN,
    200,           400,     0};
