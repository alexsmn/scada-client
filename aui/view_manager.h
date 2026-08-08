#pragma once

#if defined(UI_QT)
#include "view_manager_qt_component.h"
#endif

namespace scada::aui {

#if defined(UI_QT)
using ViewManagerComponent = ::ViewManagerQtComponent;
#endif

#if defined(UI_QT)
using ViewManagerViewId = ViewManagerComponent::ViewId;
using ViewManagerViewInfo = ViewManagerComponent::ViewInfo;
using ViewManagerLayoutNode = ViewManagerComponent::LayoutNode;
using ViewManagerSavedLayout = ViewManagerComponent::SavedLayout;
#endif

}  // namespace aui
