#pragma once

struct ScreenshotSpec;
class QWidget;

// Resizes `widget` to the spec dimensions, repaints, grabs a QPixmap
// and writes it to `GetOutputDir() / spec.filename`. No-op on null.
void SaveScreenshot(QWidget* widget, const ScreenshotSpec& spec);
