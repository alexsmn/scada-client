#include "components/web/web_component.h"

#include "components/web/web_view.h"
#include "controller/controller_registry.h"

const WindowInfo kWebWindowInfo = {ID_WEB_VIEW, "Web", u"Web"};

REGISTER_CONTROLLER(WebView, kWebWindowInfo);
