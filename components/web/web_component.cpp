#include "components/web/web_component.h"

#include "controller_registry.h"
#include "components/web/web_view.h"

const WindowInfo kWebWindowInfo = {ID_WEB_VIEW, "Web", L"Web"};

REGISTER_CONTROLLER(WebView, kWebWindowInfo);
