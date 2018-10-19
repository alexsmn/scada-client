#pragma once

#include "controls/status_bar_model.h"

#include <vector>

class QLabel;
class QProgressBar;
class QStatusBar;
class StatusBarModel;

class StatusBarController : private StatusBarModelObserver {
 public:
  StatusBarController(QStatusBar& status_bar, StatusBarModel& model);
  ~StatusBarController();

 private:
  void UpdateProgressBar();

  // StatusBarModelObserver
  virtual void OnPanesChanged(int index, int count) override;
  virtual void OnProgressChanged() override;

  QStatusBar& status_bar_;
  StatusBarModel& model_;

  std::vector<QLabel*> panes_;

  QProgressBar* progress_bar_ = nullptr;
};
