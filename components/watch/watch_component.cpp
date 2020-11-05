#include "components/watch/watch_view.h"
#include "controller_factory.h"

const WindowInfo kWindowInfo = {
    ID_WATCH_VIEW, "Log", L"Наблюдение", WIN_DISALLOW_NEW, 0, 0, 0};

REGISTER_CONTROLLER(WatchView, kWindowInfo);
