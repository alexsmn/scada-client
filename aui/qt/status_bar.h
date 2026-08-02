#pragma once

#include "aui/models/status_bar_model.h"

#include <QStatusBar>

#include <boost/signals2/connection.hpp>
#include <vector>

class QLabel;

namespace scada::aui {

// Status-bar widget over a StatusBarModel: one permanent QLabel per pane,
// refreshed on the model's panes-changed signal. The model must outlive the
// widget.
class StatusBar final : public QStatusBar {
 public:
  explicit StatusBar(StatusBarModel& model, QWidget* parent = nullptr);
  ~StatusBar();

  // Inserts |widget| left of the model panes among the permanent widgets
  // (e.g. a progress indicator). The status bar takes ownership.
  void InsertLeadingWidget(QWidget* widget);

 private:
  void OnPanesChanged(int index, int count);

  StatusBarModel& model_;

  std::vector<QLabel*> panes_;

  boost::signals2::scoped_connection panes_changed_connection_;
};

}  // namespace scada::aui
