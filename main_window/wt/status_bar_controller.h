#pragma once

#include "aui/models/status_bar_model.h"

#include <vector>

namespace aui {
class StatusBarModel;
}

class WLabel;
class WProgressBar;

class StatusBarController : private aui::StatusBarModelObserver {
 public:
  explicit StatusBarController(aui::StatusBarModel& model);
  ~StatusBarController();

 private:
  void UpdateProgressBar();

  // aui::StatusBarModelObserver
  virtual void OnPanesChanged(int index, int count) override;
  virtual void OnProgressChanged() override;

  aui::StatusBarModel& model_;

  std::vector<WLabel*> panes_;

  WProgressBar* progress_bar_ = nullptr;
};
