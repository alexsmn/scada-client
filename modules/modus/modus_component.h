#pragma once

#include "controller/window_info.h"
#include "resources/common_resources.h"

// Not WIN_REQUIRES_SCADA: a display opens on any OPC UA server, from a file
// copied into the public folder, and a binding naming a foreign node by
// namespace URI (`{nsu=…}`) resolves there
// (docs/product/foreign-node-references.md, phase 4).
inline static const WindowInfo kModusWindowInfo = {ID_MODUS_VIEW, "Modus",
                                                   u"Modus"};
