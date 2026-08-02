#include "aui/qt/status_bar.h"

#include <QLabel>

namespace scada::aui {

namespace {

// Applies an optional per-pane text colour to a status-bar label. An unset
// colour clears any previous override so the label follows the theme palette.
void ApplyPaneColor(QLabel* label, const std::optional<Color>& color) {
  label->setStyleSheet(color
                           ? QStringLiteral("QLabel{color:%1;font-weight:600;}")
                                 .arg(color->qcolor().name())
                           : QString{});
}

}  // namespace

StatusBar::StatusBar(StatusBarModel& model, QWidget* parent)
    : QStatusBar{parent}, model_{model} {
  panes_.reserve(model_.GetPaneCount());
  for (int i = 0; i < model_.GetPaneCount(); ++i) {
    auto* pane = new QLabel{this};
    pane->setMargin(2);
    pane->setText(QString::fromStdU16String(model_.GetPaneText(i)));
    ApplyPaneColor(pane, model_.GetPaneColor(i));
    addPermanentWidget(pane);
    panes_.emplace_back(pane);
  }

  panes_changed_connection_ = model_.SubscribePanesChanged(
      [this](int index, int count) { OnPanesChanged(index, count); });
}

StatusBar::~StatusBar() = default;

void StatusBar::InsertLeadingWidget(QWidget* widget) {
  insertPermanentWidget(0, widget);
}

void StatusBar::OnPanesChanged(int index, int count) {
  for (int i = 0; i < count; ++i) {
    auto text = model_.GetPaneText(index + i);
    panes_[index + i]->setText(QString::fromStdU16String(text));
    ApplyPaneColor(panes_[index + i], model_.GetPaneColor(index + i));
  }
}

}  // namespace scada::aui
