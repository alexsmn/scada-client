#include "components/watch/watch_component.h"

#include "components/watch/watch_view.h"
#include "controller_registry.h"

const WindowInfo kWatchWindowInfo = {
    ID_WATCH_VIEW, "Log", u"Наблюдение", WIN_DISALLOW_NEW, 0, 0, 0};

REGISTER_CONTROLLER(WatchView, kWatchWindowInfo);
