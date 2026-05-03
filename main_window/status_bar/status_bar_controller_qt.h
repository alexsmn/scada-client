#pragma once

#include "aui/models/status_bar_model.h"

#include <boost/signals2/connection.hpp>
#include <vector>

namespace aui {
class StatusBarModel;
}

class ProgressHost;
class QLabel;
class QProgressBar;
class QStatusBar;

class StatusBarController final : private aui::StatusBarModelObserver {
 public:
  StatusBarController(QStatusBar& status_bar,
                      aui::StatusBarModel& model,
                      ProgressHost& progress_host);
  ~StatusBarController();

  std::u16string GetPaneText(int index) const;

 private:
  void UpdateProgressBar();

  // aui::StatusBarModelObserver
  virtual void OnPanesChanged(int index, int count) override;

  QStatusBar& status_bar_;
  aui::StatusBarModel& model_;
  ProgressHost& progress_host_;

  std::vector<QLabel*> panes_;

  QProgressBar* progress_bar_ = nullptr;

  boost::signals2::scoped_connection progress_connection_;
};
