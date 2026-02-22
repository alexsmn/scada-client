#include "components/summary/summary_component.h"

#include "components/summary/summary_view.h"
#include "controller/controller_registry.h"

const WindowInfo kSummaryWindowInfo = {ID_SUMMARY_VIEW, "Summ", u"Summary",
                                       WIN_INS | WIN_CAN_PRINT};

REGISTER_CONTROLLER(SummaryView, kSummaryWindowInfo);
