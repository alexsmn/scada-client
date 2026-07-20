#pragma once

#include "aui/models/status_bar_model.h"

#include <boost/signals2/connection.hpp>
#include <vector>

namespace Wt {
class WLabel;
}  // namespace Wt

namespace scada::aui {

// Wt counterpart of the Qt StatusBar widget. Subscribes to the model's
// panes-changed signal; the widget-side rendering is not implemented yet.
class StatusBar {
 public:
  explicit StatusBar(StatusBarModel& model);
  ~StatusBar();

 private:
  void OnPanesChanged(int index, int count);

  StatusBarModel& model_;

  std::vector<Wt::WLabel*> panes_;

  boost::signals2::scoped_connection panes_changed_connection_;
};

}  // namespace scada::aui
