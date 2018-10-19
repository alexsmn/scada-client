#pragma once

#include "base/strings/string16.h"
#include "controls/status_bar_model.h"

#include <windows.h>
#include <memory>

class StatusBarController : private StatusBarModelObserver {
 public:
  StatusBarController(std::shared_ptr<StatusBarModel> model, HWND hwnd);
  ~StatusBarController();

  void Layout();

 private:
  void SetPaneText(int pane, const base::string16& text);

  // StatusBarModelObserver
  virtual void OnPanesChanged(int index, int count) override;
  virtual void OnProgressChanged() override;

  const std::shared_ptr<StatusBarModel> model_;

  HWND hwnd_ = nullptr;
};
