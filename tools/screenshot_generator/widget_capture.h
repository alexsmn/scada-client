#pragma once

struct ScreenshotSpec;
class QWidget;

#include <QPixmap>

// Grabs `widget` repeatedly until it renders the same frame several times in a
// row, and returns that frame.
//
// Use this in place of `widget->grab()` after a fixed `PumpEventLoopFor(...)`.
// A fixed pump is a shared budget — a run capturing many windows settles each
// one less than a single-window run does — so the same spec could render with
// or without its trends depending on what else the run contained, and nothing
// failed when it did. Pair it with WaitForPendingData(), which waits for the
// data itself; this only waits for the repaint that follows.
//
// Reports a test failure (and returns the last frame) if the widget never
// stops changing, which would mean something is animating independently of the
// generator's frozen clock.
//
// The give-up point is a number of frames compared, not an elapsed time, so a
// loaded machine makes this slower but never makes it fail. See kMaxFrames in
// settle_loop.h.
QPixmap GrabWhenSettled(QWidget* widget);

// Resizes `widget` to the spec dimensions, waits for it to settle, grabs a
// QPixmap and writes it to `GetOutputDir() / spec.filename`. No-op on null.
void SaveScreenshot(QWidget* widget, const ScreenshotSpec& spec);
