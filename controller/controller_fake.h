#pragma once

#include "controller/controller.h"

#if defined(UI_QT)
#include <QWidget>
#endif

class FakeController final : public Controller {
 public:
  virtual std::unique_ptr<UiView> Init(
      const WindowDefinition& definition) override {
    return std::make_unique<QWidget>();
  }
};
