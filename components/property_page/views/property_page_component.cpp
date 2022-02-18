#include "components/property_page/views/property_page_view.h"
#include "controller_registry.h"

const WindowInfo kWindowInfo = {ID_PROPERTY_VIEW, "RecEditor", u"Параметры",
                                WIN_DISALLOW_NEW | WIN_REQUIRES_ADMIN};

REGISTER_CONTROLLER(PropertyPageView, kWindowInfo);
