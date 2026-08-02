#include "widget_capture.h"

#include "screenshot_config.h"
#include "screenshot_output.h"
#include "screenshot_wait.h"

#include <gtest/gtest.h>

#include <QApplication>
#include <QElapsedTimer>
#include <QImage>
#include <QPixmap>
#include <QString>
#include <QWidget>

#include <chrono>

namespace {

// Consecutive identical frames that count as settled, and the event-loop time
// between them.
constexpr int kSettledFrames = 3;
constexpr auto kFrameInterval = std::chrono::milliseconds{120};
constexpr auto kSettleTimeout = std::chrono::seconds{10};

}  // namespace

// Works only because nothing in a capture animates on its own: blink phase is
// derived from the clock (see BlinkPhaseAt) and the generator freezes the
// clock, so a widget that stops changing has genuinely finished.
QPixmap GrabWhenSettled(QWidget* widget) {
  QElapsedTimer elapsed;
  elapsed.start();

  QImage previous;
  QPixmap pixmap;
  int settled = 0;

  for (;;) {
    widget->repaint();
    pixmap = widget->grab();

    QImage frame = pixmap.toImage();
    settled = (!previous.isNull() && frame == previous) ? settled + 1 : 0;
    previous = std::move(frame);

    if (settled >= kSettledFrames)
      return pixmap;

    if (elapsed.hasExpired(std::chrono::milliseconds{kSettleTimeout}.count())) {
      ADD_FAILURE() << "Widget never stopped changing within "
                    << kSettleTimeout.count()
                    << "s; the capture is not reproducible. Something is "
                       "animating independently of the frozen fixture clock.";
      return pixmap;
    }

    scada::screenshot_generator::PumpEventLoopFor(kFrameInterval);
  }
}

void SaveScreenshot(QWidget* widget, const ScreenshotSpec& spec) {
  if (!widget)
    return;

  // On-page views are laid-out children (docked / tabbed), so resize() alone is
  // immediately overridden by the parent layout and grab() captures the
  // layout-controlled size rather than the requested spec size — a sparse page
  // (e.g. a --only run with few windows) tiles them narrow. Detach the widget to
  // a top-level for the grab so the requested dimensions stick, then restore its
  // parent so the widget tree is left as we found it.
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
