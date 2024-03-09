#pragma once

#if defined(UI_QT)
#include <QRect>
#elif defined(UI_WT)
#include "aui/rect_internal.h"
#endif

namespace aui {

#if defined(UI_QT)
using Rect = QRect;
#elif defined(UI_WT)
using Rect = internal::Rect;
#endif

}  // namespace aui
