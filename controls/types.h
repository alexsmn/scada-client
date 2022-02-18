#pragma once

#if defined(UI_QT)
class QWidget;
typedef QWidget UiView;
#elif defined(UI_VIEWS)
namespace views {
class View;
}
typedef views::View UiView;
#elif defined(UI_WT)
namespace Wt {
class WWidget;
}
typedef Wt::WWidget UiView;
#endif
