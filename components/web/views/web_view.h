#pragma once

#include "controller.h"
#include "controller_context.h"
#include "ui/views/controls/activex_control.h"

class WebView : protected ControllerContext,
                public views::ActiveXControl,
                public Controller {
 public:
  explicit WebView(const ControllerContext& context);

  // Controller
  virtual views::View* Init(const WindowDefinition& definition) override;

 protected:
  // Controller
  virtual void Save(WindowDefinition& definition) override;

  // NativeControl
  virtual void NativeControlCreated(HWND window_handle) override;

 private:
  std::wstring url_;
};
