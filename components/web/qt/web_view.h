#pragma once

#include "base/files/file_path.h"
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

  base::FilePath path_;
};
