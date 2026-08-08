#pragma once

#if defined(UI_QT)
#include <QPoint>
#endif

namespace scada::aui {

#if defined(UI_QT)
typedef QPoint Point;
#endif

}  // namespace aui
