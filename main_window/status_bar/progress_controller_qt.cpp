#include "main_window/status_bar/progress_controller_qt.h"

#include "aui/qt/status_bar.h"
#include "core/progress_host.h"

#include <QProgressBar>

ProgressController::ProgressController(scada::aui::StatusBar& status_bar,
                                       ProgressHost& progress_host)
    : progress_host_{progress_host} {
  // cppcheck-suppress noCopyConstructor
  // cppcheck-suppress noOperatorEq
  progress_bar_ = new QProgressBar{&status_bar};
  progress_bar_->setAlignment(Qt::AlignRight);
  progress_bar_->setRange(0, 0);
  progress_bar_->setMaximumWidth(200);
  UpdateProgressBar();
  status_bar.InsertLeadingWidget(progress_bar_);

  progress_connection_ = progress_host.Subscribe(
      [this](const ProgressStatus&) { UpdateProgressBar(); });
}

ProgressController::~ProgressController() = default;

void ProgressController::UpdateProgressBar() {
  auto progress = progress_host_.GetStatus();
  progress_bar_->setRange(0, progress.range);
  progress_bar_->setValue(progress.current);
  progress_bar_->setVisible(progress.active);
}
