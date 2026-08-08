#pragma once

#if defined(UI_QT)
#include <QRect>
#endif

namespace scada::aui {

#if defined(UI_QT)
using Rect = QRect;
#endif

}  // namespace aui
