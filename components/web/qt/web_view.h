#pragma once

#include "controller.h"

class QAxWidget;

class WebView : public Controller {
 public:
  explicit WebView(const ControllerContext& context);
  ~WebView();

  // Controller
  virtual UiView* Init(const WindowDefinition& definition) override;
  virtual void Save(WindowDefinition& definition) override;

 private:
  std::unique_ptr<QAxWidget> ax_widget_;

  std::wstring url_;
};
