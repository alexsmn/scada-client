#pragma once

#include "base/strings/string16.h"
#include "controls/status_bar_model.h"

#include <windows.h>
#include <memory>

class StatusBarController : private StatusBarModelObserver {
 public:
  explicit StatusBarController(std::shared_ptr<StatusBarModel> model);
  ~StatusBarController();

  void Init(HWND hwnd);

  void Layout();

 private:
  void SetPaneText(int pane, const base::string16& text);

  // StatusBarModelObserver
  virtual void OnPanesChanged(int index, int count) override;

  const std::shared_ptr<StatusBarModel> model_;

  HWND hwnd_ = nullptr;
};
