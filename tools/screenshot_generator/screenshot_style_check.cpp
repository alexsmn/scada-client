// The capture style must not hide a control the operator can see: this pins
// the idle line-edit frame without rendering a published image.

#include "screenshot_fixture.h"

#include <gtest/gtest.h>

#include <QApplication>
#include <QImage>
#include <QLineEdit>
#include <QPalette>
#include <QPixmap>

#include <cstdlib>

namespace {

using scada::screenshot_generator::ScreenshotGenerator;

bool Near(const QColor& a, const QColor& b) {
  constexpr int kTolerance = 3;
  return std::abs(a.red() - b.red()) <= kTolerance &&
         std::abs(a.green() - b.green()) <= kTolerance &&
         std::abs(a.blue() - b.blue()) <= kTolerance;
}

}  // namespace

// Backlog 793: under plain Fusion an idle text field's frame is the window
// colour darkened by 140 %, which on the dark window is (21,21,21) against
// (30,30,30) — no boundary at all, in every dark capture with a bare
// QLineEdit (workbench-login.png's password field was the one measured). The
// capture style strokes it from QPalette::Mid, which the theme lifts to 3:1.
TEST_F(ScreenshotGenerator, IdleLineEditFrameIsDrawnFromMid) {
  QLineEdit edit;
  edit.resize(160, 24);
  ASSERT_FALSE(edit.hasFocus());

  const QImage image = edit.grab().toImage();
  ASSERT_FALSE(image.isNull());
  const qreal dpr = image.devicePixelRatio();
  // The middle of the left edge: on the frame's straight run, clear of the
  // rounded corners and of the inner top shadow.
  const QColor frame = image.pixelColor(0, static_cast<int>(12 * dpr));

  const QColor mid = QApplication::palette().color(QPalette::Mid);
  EXPECT_TRUE(Near(frame, mid)) << "frame " << frame.name().toStdString()
                                << ", Mid " << mid.name().toStdString();
  EXPECT_FALSE(
      Near(frame, QApplication::palette().color(QPalette::Window).darker(140)))
      << "the frame is still Fusion's own outline";
}
