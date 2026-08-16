#pragma once

#include "screenshot_config.h"

#include <QtCore/QElapsedTimer>
#include <QtWidgets/QApplication>

#include <set>

namespace scada::aui {
class Tree;
}

class MainWindow;
class NodeService;
class OpenedView;
class QWidget;
class TimedDataService;

namespace boost::json {
class value;
}

namespace scada::screenshot_generator {

// Pumps the Qt event loop until `predicate` holds or `timeout_ms` elapses,
// returning whether it held. Lives here rather than in screenshot_wait.h
// because it is the processEvents-based form: it does not fire timers on a
// drained queue (macOS), so anything waiting on a scheduled continuation wants
// PumpEventLoopFor instead.
template <class Predicate>
bool WaitUntil(Predicate&& predicate, int timeout_ms = 5000) {
  QElapsedTimer timer;
  timer.start();
  while (!predicate()) {
    if (timer.elapsed() >= timeout_ms)
      return false;
    QApplication::processEvents(QEventLoop::AllEvents, 50);
  }
  return true;
}

// The aui tree a view renders into — the widget itself, or the first one
// below it. dynamic_cast rather than findChild<>, because Tree is not a
// QObject-registered type at every level.
scada::aui::Tree* FindTreeWidget(QWidget* widget);

// Everything the generic view capture needs from the fixture. Bundled so the
// procedure below can move out of the test body without growing a parameter
// list nobody can read.
struct ViewCaptureContext {
  MainWindow& main_window;
  NodeService& node_service;
  TimedDataService& timed_data_service;
  const boost::json::value& json;
};

// Captures one page-backed spec: selects the owning activity-rail mode, finds
// the spec's not-yet-consumed opened view, warms the spec's own data items,
// asserts the view rendered the rows/columns/cells the fixture defines, and
// grabs it.
//
// `used_views` carries across calls within one sweep so that several specs
// sharing a window_type consume successive views in page order; the caller
// clears it when a pane-mode switch invalidates the pointers.
//
// Returns whether a capture was written. A false return has already reported
// its own gtest failure.
bool CaptureViewSpec(const ScreenshotSpec& spec,
                     const ViewCaptureContext& context,
                     std::set<const OpenedView*>& used_views);

}  // namespace scada::screenshot_generator
