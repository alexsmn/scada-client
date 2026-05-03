#include "modules/watch/watch_component.h"

#include "modules/watch/watch_view.h"
#include "controller/controller_registry.h"

const WindowInfo kWatchWindowInfo = {
    ID_WATCH_VIEW, "Log", u"Watch", WIN_DISALLOW_NEW, 0, 0, 0};

REGISTER_CONTROLLER(WatchView, kWatchWindowInfo);
