#pragma once

#include "aui/models/status_bar_model.h"

#include <boost/signals2/connection.hpp>
#include <vector>

namespace aui {
class StatusBarModel;
}

class ProgressHost;
class WLabel;
class WProgressBar;

class StatusBarController {
 public:
  StatusBarController(aui::StatusBarModel& model, ProgressHost& progress_host);
  ~StatusBarController();

 private:
  void UpdateProgressBar();

  void OnPanesChanged(int index, int count);

  aui::StatusBarModel& model_;
  ProgressHost& progress_host_;

  std::vector<WLabel*> panes_;

  WProgressBar* progress_bar_ = nullptr;

  boost::signals2::scoped_connection panes_changed_connection_;
};
