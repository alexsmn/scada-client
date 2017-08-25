#pragma once

#include "client/controller.h"
#include "ui/views/controls/activex_control.h"

class WebView : public views::ActiveXControl,
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
