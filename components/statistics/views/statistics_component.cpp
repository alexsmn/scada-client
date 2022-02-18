#include "components/statistics/views/statistics_view.h"
#include "controller_registry.h"

const WindowInfo kWindowInfo = {ID_STATISTICS_VIEW, "Stat", u"Статус",
                                WIN_SING,           300,    400};

REGISTER_CONTROLLER(StatisticsView, kWindowInfo);
