#include "portfolio/portfolio_component.h"

#include "portfolio/portfolio_view.h"
#include "controller/controller_registry.h"

const WindowInfo kPortfolioWindowInfo = {ID_PORTFOLIO_VIEW,
                                         "Portfolio",
                                         u"Портфолио",
                                         WIN_SING | WIN_INS,
                                         200,
                                         400,
                                         0};

REGISTER_CONTROLLER(PortfolioView, kPortfolioWindowInfo);
