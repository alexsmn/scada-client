#include "aui/wt/status_bar.h"

#include <Wt/WLabel.h>

namespace scada::aui {

StatusBar::StatusBar(StatusBarModel& model) : model_{model} {
  /*panes_.reserve(model_.GetPaneCount());
  for (int i = 0; i < model_.GetPaneCount(); ++i) {
    auto* pane = new QLabel{&status_bar_};
    pane->setMargin(2);
    pane->setText(QString::fromStdWString(model_.GetPaneText(i)));
    status_bar_.addPermanentWidget(pane);
    panes_.emplace_back(pane);
  }*/

  panes_changed_connection_ = model_.SubscribePanesChanged(
      [this](int index, int count) { OnPanesChanged(index, count); });
}

StatusBar::~StatusBar() = default;

void StatusBar::OnPanesChanged(int index, int count) {
  /*for (int i = 0; i < count; ++i)
    panes_[i]->setText(QString::fromStdWString(model_.GetPaneText(i)));*/
}

}  // namespace scada::aui
