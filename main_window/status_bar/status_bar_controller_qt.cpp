#include "status_bar_controller_qt.h"

#include "aui/models/status_bar_model.h"
#include "core/progress_host.h"

#include <QLabel>
#include <QProgressBar>
#include <QStatusBar>

namespace {

// Applies an optional per-pane text colour to a status-bar label. An unset
// colour clears any previous override so the label follows the theme palette.
void ApplyPaneColor(QLabel* label, const std::optional<aui::Color>& color) {
  label->setStyleSheet(color
                           ? QStringLiteral("QLabel{color:%1;font-weight:600;}")
                                 .arg(color->qcolor().name())
                           : QString{});
}

}  // namespace

StatusBarController::StatusBarController(QStatusBar& status_bar,
                                         aui::StatusBarModel& model,
                                         ProgressHost& progress_host)
    : status_bar_{status_bar}, model_{model}, progress_host_{progress_host} {
  // cppcheck-suppress noCopyConstructor
  // cppcheck-suppress noOperatorEq
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
    pane->setText(QString::fromStdU16String(model_.GetPaneText(i)));
    ApplyPaneColor(pane, model_.GetPaneColor(i));
    status_bar_.addPermanentWidget(pane);
    panes_.emplace_back(pane);
  }

  panes_changed_connection_ = model_.SubscribePanesChanged(
      [this](int index, int count) { OnPanesChanged(index, count); });

  progress_connection_ = progress_host.Subscribe(
      [this](const ProgressStatus&) { UpdateProgressBar(); });
}

StatusBarController::~StatusBarController() = default;

void StatusBarController::OnPanesChanged(int index, int count) {
  for (int i = 0; i < count; ++i) {
    auto text = model_.GetPaneText(index + i);
    panes_[index + i]->setText(QString::fromStdU16String(text));
    ApplyPaneColor(panes_[index + i], model_.GetPaneColor(index + i));
  }
}

void StatusBarController::UpdateProgressBar() {
  auto progress = progress_host_.GetStatus();
  progress_bar_->setRange(0, progress.range);
  progress_bar_->setValue(progress.current);
  progress_bar_->setVisible(progress.active);
}

std::u16string StatusBarController::GetPaneText(int index) const {
  return panes_[index]->text().toStdU16String();
}
