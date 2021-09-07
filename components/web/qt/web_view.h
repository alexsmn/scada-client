#pragma once

#include "base/files/file_path.h"
#include "controller.h"
#include "controller_context.h"

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

  base::FilePath path_;
};
