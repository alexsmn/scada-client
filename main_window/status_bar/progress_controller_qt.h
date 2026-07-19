#pragma once

#include "aui/aui_ns_compat.h"

#include <boost/signals2/connection.hpp>

class ProgressHost;
class QProgressBar;

namespace scada::aui {
class StatusBar;
}

// Shows the ProgressHost's aggregated progress as an indicator leading the
// status bar's panes, visible while an operation is active.
class ProgressController final {
 public:
  ProgressController(aui::StatusBar& status_bar, ProgressHost& progress_host);
  ~ProgressController();

 private:
  void UpdateProgressBar();

  ProgressHost& progress_host_;

  QProgressBar* progress_bar_ = nullptr;

  boost::signals2::scoped_connection progress_connection_;
};
