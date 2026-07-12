#pragma once

#include "aui/aui_ns_compat.h"

#if defined(UI_QT)
#include <QPoint>
#elif defined(UI_WT)
#include <Wt/WPoint.h>
#endif

namespace scada::aui {

#if defined(UI_QT)
typedef QPoint Point;
#elif defined(UI_WT)
typedef Wt::WPoint Point;
#endif

}  // namespace aui
