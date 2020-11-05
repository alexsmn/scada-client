#include "components/property_page/views/property_page_view.h"
#include "controller_factory.h"

const WindowInfo kWindowInfo = {ID_PROPERTY_VIEW, "RecEditor", L"Параметры",
                                WIN_DISALLOW_NEW | WIN_REQUIRES_ADMIN};

REGISTER_CONTROLLER(PropertyPageView, kWindowInfo);
