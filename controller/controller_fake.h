#pragma once

#include "controller/controller.h"

#if defined(UI_QT)
#include <QWidget>
#elif defined(UI_WT)
#include <wt/WContainerWidget.h>
#endif

class FakeController final : public Controller {
 public:
  virtual std::unique_ptr<UiView> Init(
      const WindowDefinition& definition) override {
#if defined(UI_QT)
    return std::make_unique<QWidget>();
#elif defined(UI_WT) 
    return std::make_unique<Wt::WContainerWidget>();
#else
    assert(false);
    return nullptr;
#endif
  }
};
