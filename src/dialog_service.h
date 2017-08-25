#pragma once

#if defined(UI_VIEWS)
#include "ui/gfx/native_widget_types.h"
#endif

#if defined(UI_QT)
class QWidget;
#endif

class DialogService {
 public:
  virtual ~DialogService() {}
};

#if defined(UI_VIEWS)
class DialogServiceViews : public DialogService {
 public:
  using DialogParentType = gfx::NativeView;

  virtual DialogParentType GetParentView() = 0;
};
#endif

#if defined(UI_QT)
class DialogServiceQt : public DialogService {
 public:
  using DialogParentType = QWidget*;

  virtual DialogParentType GetParentView() = 0;
};
#endif
