#pragma once

#include "aui/aui_ns_compat.h"

#if defined(UI_QT)
#include <QRect>
#elif defined(UI_WT)
#include "aui/rect_internal.h"
#endif

namespace scada::aui {

#if defined(UI_QT)
using Rect = QRect;
#elif defined(UI_WT)
using Rect = internal::Rect;
#endif

}  // namespace aui
