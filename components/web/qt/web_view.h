#pragma once

#include "controller/controller.h"
#include "controller/controller_context.h"

#include <filesystem>

class QAxWidget;

class WebView : protected ControllerContext, public Controller {
 public:
  explicit WebView(const ControllerContext& context);
  ~WebView();

  // Controller
  virtual UiView* Init(const WindowDefinition& definition) override;
  virtual void Save(WindowDefinition& definition) override;

 private:
  QAxWidget* ax_widget_ = nullptr;

  std::filesystem::path path_;
};
