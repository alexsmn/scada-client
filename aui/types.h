#pragma once

#if defined(UI_QT)

class QWidget;

using UiView = QWidget;

#elif defined(UI_WT)

namespace Wt {
class WWidget;
}

using UiView = Wt::WWidget;

#endif
