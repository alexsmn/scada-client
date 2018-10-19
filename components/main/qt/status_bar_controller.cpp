#include "status_bar_controller.h"

#include "controls/status_bar_model.h"

#include <QLabel>
#include <QStatusBar>
#include <QProgressBar>

StatusBarController::StatusBarController(QStatusBar& status_bar,
                                         StatusBarModel& model)
    : status_bar_{status_bar}, model_{model} {
  progress_bar_ = new QProgressBar{&status_bar_};
  progress_bar_->setAlignment(Qt::AlignRight);
  progress_bar_->setRange(0, 0);
  progress_bar_->setMaximumWidth(200);
  UpdateProgressBar();
  status_bar_.addPermanentWidget(progress_bar_);

  panes_.reserve(model_.GetPaneCount());
  for (int i = 0; i < model_.GetPaneCount(); ++i) {
    auto* pane = new QLabel{&status_bar_};
    pane->setMargin(2);
    pane->setText(QString::fromStdWString(model_.GetPaneText(i)));
    status_bar_.addPermanentWidget(pane);
    panes_.emplace_back(pane);
  }

  model_.AddObserver(*this);
}

StatusBarController::~StatusBarController() {
  model_.RemoveObserver(*this);
}

void StatusBarController::OnPanesChanged(int index, int count) {
  for (int i = 0; i < count; ++i)
    panes_[i]->setText(QString::fromStdWString(model_.GetPaneText(i)));
}

void StatusBarController::UpdateProgressBar() {
  auto progress = model_.GetProgress();
  progress_bar_->setRange(0, progress.range);
  progress_bar_->setValue(progress.current);
  progress_bar_->setVisible(progress.active);
}

void StatusBarController::OnProgressChanged() {
  UpdateProgressBar();
}
