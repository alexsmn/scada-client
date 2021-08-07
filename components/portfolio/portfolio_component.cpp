#include "components/portfolio/portfolio_component.h"

#include "components/portfolio/portfolio_view.h"
#include "controller_factory.h"

const WindowInfo kPortfolioWindowInfo = {ID_PORTFOLIO_VIEW,
                                         "Portfolio",
                                         L"Портфолио",
                                         WIN_SING | WIN_INS,
                                         200,
                                         400,
                                         0};

REGISTER_CONTROLLER(PortfolioView, kPortfolioWindowInfo);
