#pragma once

#include "controls/status_bar_model.h"

#include <vector>

class WLabel;
class WProgressBar;
class StatusBarModel;

class StatusBarController : private StatusBarModelObserver {
 public:
  StatusBarController(StatusBarModel& model);
  ~StatusBarController();

 private:
  void UpdateProgressBar();

  // StatusBarModelObserver
  virtual void OnPanesChanged(int index, int count) override;
  virtual void OnProgressChanged() override;

  StatusBarModel& model_;

  std::vector<WLabel*> panes_;

  WProgressBar* progress_bar_ = nullptr;
};
