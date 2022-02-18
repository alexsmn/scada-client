#pragma once

#include "controls/status_bar_model.h"

#include <memory>
#include <string>
#include <windows.h>

class StatusBarController : private StatusBarModelObserver {
 public:
  StatusBarController(std::shared_ptr<StatusBarModel> model, HWND hwnd);
  ~StatusBarController();

  void Layout();

 private:
  void SetPaneText(int pane, const std::u16string& text);

  // StatusBarModelObserver
  virtual void OnPanesChanged(int index, int count) override;
  virtual void OnProgressChanged() override;

  const std::shared_ptr<StatusBarModel> model_;

  HWND hwnd_ = nullptr;
};
