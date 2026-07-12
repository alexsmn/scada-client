#pragma once

#include "aui/aui_ns_compat.h"

#include "aui/models/status_bar_model.h"

#include <boost/signals2/connection.hpp>
#include <vector>

namespace scada::aui {
class StatusBarModel;
}

class ProgressHost;
class QLabel;
class QProgressBar;
class QStatusBar;

class StatusBarController final {
 public:
  StatusBarController(QStatusBar& status_bar,
                      aui::StatusBarModel& model,
                      ProgressHost& progress_host);
  ~StatusBarController();

  std::u16string GetPaneText(int index) const;

 private:
  void UpdateProgressBar();

  void OnPanesChanged(int index, int count);

  QStatusBar& status_bar_;
  aui::StatusBarModel& model_;
  ProgressHost& progress_host_;

  std::vector<QLabel*> panes_;

  QProgressBar* progress_bar_ = nullptr;

  boost::signals2::scoped_connection progress_connection_;
  boost::signals2::scoped_connection panes_changed_connection_;
};
