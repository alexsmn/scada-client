#pragma once

#if defined(UI_QT)
#include <QPoint>
#elif defined(UI_WT)
#include <Wt/WPoint.h>
#endif

namespace aui {

#if defined(UI_QT)
typedef QPoint Point;
#elif defined(UI_WT)
typedef Wt::WPoint Point;
#endif

}  // namespace aui
