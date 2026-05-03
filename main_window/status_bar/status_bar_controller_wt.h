#pragma once

#include "aui/models/status_bar_model.h"

#include <vector>

namespace aui {
class StatusBarModel;
}

class ProgressHost;
class WLabel;
class WProgressBar;

class StatusBarController : private aui::StatusBarModelObserver {
 public:
  StatusBarController(aui::StatusBarModel& model, ProgressHost& progress_host);
  ~StatusBarController();

 private:
  void UpdateProgressBar();

  // aui::StatusBarModelObserver
  virtual void OnPanesChanged(int index, int count) override;

  aui::StatusBarModel& model_;
  ProgressHost& progress_host_;

  std::vector<WLabel*> panes_;

  WProgressBar* progress_bar_ = nullptr;
};
