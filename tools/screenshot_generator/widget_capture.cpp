#include "widget_capture.h"

#include "screenshot_config.h"
#include "screenshot_output.h"
#include "screenshot_wait.h"
#include "settle_loop.h"

#include <gtest/gtest.h>

#include <QApplication>
#include <QDockWidget>
#include <QElapsedTimer>
#include <QImage>
#include <QLayout>
#include <QPixmap>
#include <QSize>
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

void SaveWindowScreenshot(QWidget* window,
                          QWidget* view,
                          const ScreenshotSpec& spec) {
  if (!window)
    return;

  // The sweep builds its main window hidden (`MainWindow::SetHideForTesting`
  // in screenshot_fixture.cpp), and a hidden window does not lay its children
  // out — the Graph branch in view_capture.cpp exists for that same reason. So
  // show it, let the layouts run, grab, and hide it again so the specs after
  // this one see the window they expected.
  //
  // What this CANNOT produce is the OS title bar: `grab()` renders the Qt
  // widget tree and the title bar belongs to the window manager.
  // `screenshot_fixture.cpp` records that PrintWindow(PW_RENDERFULLCONTENT)
  // was tried for precisely this and did not reliably capture child content.
  // Restore the window's geometry afterwards, not just its visibility. The
  // sweep renders every spec into ONE main window, so resizing it here and
  // leaving it resized makes the NEXT capture lay out against this spec's
  // dimensions — measured: `files.png` rendered after this capture differed
  // from the same spec rendered alone, which is the order-dependence the
  // fixture's own comments record being bitten by before.
  const bool was_visible = window->isVisible();
  const QSize previous_size = window->size();

  // Raise the pane being captured, so the window is shown around THIS view
  // rather than whichever dock happened to be on top.
  if (view) {
    if (auto* dock = qobject_cast<QDockWidget*>(view->parentWidget())) {
      dock->show();
      dock->raise();
    }
  }

  window->resize(spec.width, spec.height);
  window->show();
  window->ensurePolished();
  if (QLayout* layout = window->layout())
    layout->activate();
  for (int i = 0; i < 20; ++i)
    QApplication::processEvents();

  QPixmap pixmap = GrabWhenSettled(window);

  if (!was_visible)
    window->hide();
  window->resize(previous_size);
  QApplication::processEvents();

  auto path = OutputPathFor(spec.filename);
  pixmap.save(QString::fromStdString(path.string()));
}

void SaveFramedScreenshot(QWidget* framed, const ScreenshotSpec& spec) {
  if (!framed)
    return;

  // Deliberately NOT the detach-and-resize path `SaveScreenshot` takes below.
  // A QDockWidget draws its title bar and its float/close buttons as children
  // only while it is docked; `setParent(nullptr)` promotes it to a top-level
  // window, where that chrome becomes the window manager's job — and under
  // `QT_QPA_PLATFORM=offscreen` there is no window manager, so the grab comes
  // back as the bare content with the frame silently gone. That failure is
  // invisible: a valid PNG of the right size, just not of the thing asked for.
  //
  // So resize in place and grab in place. The size is a request rather than a
  // guarantee — the dock's layout in the main window has the final say — which
  // is the cost of keeping the frame.
  framed->setMinimumSize(spec.width, spec.height);
  framed->resize(spec.width, spec.height);
  QApplication::processEvents();

  QPixmap pixmap = GrabWhenSettled(framed);
  framed->setMinimumSize(0, 0);

  auto path = OutputPathFor(spec.filename);
  pixmap.save(QString::fromStdString(path.string()));
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

  auto path = OutputPathFor(spec.filename);
  pixmap.save(QString::fromStdString(path.string()));
}
