#include "widget_capture.h"

#include "screenshot_config.h"
#include "screenshot_output.h"
#include "screenshot_wait.h"
#include "settle_loop.h"

#include <gtest/gtest.h>

#include <QApplication>
#include <QElapsedTimer>
#include <QImage>
#include <QPixmap>
#include <QString>
#include <QWidget>

#include <chrono>

// Works only because nothing in a capture animates on its own: blink phase is
// derived from the clock (see BlinkPhaseAt) and the generator freezes the
// clock, so a widget that stops changing has genuinely finished.
//
// The loop itself is RunSettleLoop, which bounds the wait by frames compared
// rather than by elapsed time — see the comment on kMaxFrames for why that
// distinction is the difference between a capture that fails under load and
// one that merely takes longer.
QPixmap GrabWhenSettled(QWidget* widget) {
  namespace sg = scada::screenshot_generator;

  QElapsedTimer elapsed;
  elapsed.start();

  QPixmap pixmap;
  const sg::SettleResult result = sg::RunSettleLoop(
      [&] {
        widget->repaint();
        pixmap = widget->grab();
        return pixmap.toImage();
      },
      [] { sg::PumpEventLoopFor(sg::kFrameInterval); });

  if (!result.settled) {
    ADD_FAILURE()
        << "Widget never stopped changing over " << result.frames << " frames ("
        << elapsed.elapsed()
        << " ms). The capture is not reproducible: something is changing "
           "independently of the frozen fixture clock, or data is still "
           "arriving. Machine load is NOT the cause — the budget is counted "
           "in frames, not seconds — but it does inflate that wall time, so a "
           "figure far above "
        << sg::kMaxFrames * sg::kFrameInterval.count()
        << " ms means the run was heavily loaded as well.";
  }
  return pixmap;
}

void SaveScreenshot(QWidget* widget, const ScreenshotSpec& spec) {
  if (!widget)
    return;

  // On-page views are laid-out children (docked / tabbed), so resize() alone is
  // immediately overridden by the parent layout and grab() captures the
  // layout-controlled size rather than the requested spec size — a sparse page
  // (e.g. a --only run with few windows) tiles them narrow. Detach the widget
  // to a top-level for the grab so the requested dimensions stick, then restore
  // its parent so the widget tree is left as we found it.
  QWidget* const parent = widget->parentWidget();
  const bool was_visible = widget->isVisible();
  if (parent)
    widget->setParent(nullptr);

  widget->resize(spec.width, spec.height);
  QApplication::processEvents();

  // Settle at the capture geometry, not the layout-controlled one: a row's
  // sparkline window and any lazily-fetched cell only reach their final state
  // once the widget has been laid out the way it will be grabbed.
  QPixmap pixmap = GrabWhenSettled(widget);

  if (parent) {
    widget->setParent(parent);
    widget->setVisible(was_visible);
  }

  auto path = GetOutputDir() / spec.filename;
  pixmap.save(QString::fromStdString(path.string()));
}
