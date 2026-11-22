#pragma once

class QStyle;

namespace scada::screenshot_generator {

// The style every capture renders under: Fusion, so a published image is
// byte-comparable across the machines that render it, with one correction.
//
// Fusion strokes a line edit's frame with its private `outline()` colour,
// which is the window colour darkened by 140 % and reads no other palette
// role (qfusionstyle_p_p.h, Qt 6.11.3). On the dark window that is (21,21,21)
// against (30,30,30), so every idle text field in the dark gallery rendered
// with no boundary at all — while the native styles operators actually run
// show it (backlog 793). This style strokes the idle frame from
// `QPalette::Mid` instead, which `BuildThemePalette` lifts to the 3:1
// non-text contrast floor; the focused frame keeps Fusion's highlight.
//
// Ownership passes to the caller (QApplication::setStyle takes it).
QStyle* MakeScreenshotStyle();

}  // namespace scada::screenshot_generator
