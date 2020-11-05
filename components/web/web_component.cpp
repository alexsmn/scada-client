#include "controller_factory.h"

#if defined(UI_QT)
#include "components/web/qt/web_view.h"
#elif defined(UI_VIEWS)
#include "components/web/views/web_view.h"
#endif

const WindowInfo kWindowInfo = {ID_WEB_VIEW, "Web", L"Web"};

REGISTER_CONTROLLER(WebView, kWindowInfo);
