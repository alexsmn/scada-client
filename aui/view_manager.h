#pragma once

#if defined(UI_QT) && defined(UI_WT)
#error "Only one view manager UI backend can be selected."
#endif

#if defined(UI_QT)
#include "view_manager_qt_component.h"
#elif defined(UI_WT)
#include "aui/wt/view_manager_wt_component.h"
#endif

namespace aui {

#if defined(UI_QT)
using ViewManagerComponent = ::ViewManagerQtComponent;
#elif defined(UI_WT)
using ViewManagerComponent = ::ViewManagerWtComponent;
#endif

#if defined(UI_QT) || defined(UI_WT)
using ViewManagerViewId = ViewManagerComponent::ViewId;
using ViewManagerViewInfo = ViewManagerComponent::ViewInfo;
using ViewManagerLayoutNode = ViewManagerComponent::LayoutNode;
using ViewManagerSavedLayout = ViewManagerComponent::SavedLayout;
#endif

}  // namespace aui
