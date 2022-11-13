#pragma once

#include "controls/models/status_bar_model.h"

#include <vector>

namespace aui {
class StatusBarModel;
}

class QLabel;
class QProgressBar;
class QStatusBar;

class StatusBarController : private aui::StatusBarModelObserver {
 public:
  StatusBarController(QStatusBar& status_bar, aui::StatusBarModel& model);
  ~StatusBarController();

 private:
  void UpdateProgressBar();

  // aui::StatusBarModelObserver
  virtual void OnPanesChanged(int index, int count) override;
  virtual void OnProgressChanged() override;

  QStatusBar& status_bar_;
  aui::StatusBarModel& model_;

  std::vector<QLabel*> panes_;

  QProgressBar* progress_bar_ = nullptr;
};
