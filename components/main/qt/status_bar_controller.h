#pragma once

#include "controls/status_bar_model.h"

#include <vector>

class QLabel;
class QStatusBar;
class StatusBarModel;

class StatusBarController : private StatusBarModelObserver {
 public:
  StatusBarController(QStatusBar& status_bar, StatusBarModel& model);
  ~StatusBarController();

 private:
  // StatusBarModelObserver
  virtual void OnPanesChanged(int index, int count) override;

  QStatusBar& status_bar_;
  StatusBarModel& model_;

  std::vector<QLabel*> panes_;
};
