#pragma once

#include <boost/json/value.hpp>

struct ScreenshotSpec;

// Renders the reshelled substation display — a cross-platform VdsRuntimeWidget
// wrapped in the DisplayFrame chrome (Live indicator, breadcrumb, zoom / fit /
// export toolbar) — from the fixture named by the manifest's `display.path`,
// and saves it under `GetOutputDir() / spec.filename`.
//
// Standalone like SaveGraphScreenshot: it builds a fresh top-level widget
// rather than reaching into the hidden main window (whose layout is skipped
// while hidden). The VDS renderer paints to a QImage independent of the window
// system, so this works headless (Qt `offscreen`), which is why it is a valid
// cross-platform (macOS/Linux) validation of the reshell frame even though the
// Windows-only Modus/Vidicon ActiveX renderer is not exercised here.
void SaveDisplayScreenshot(const ScreenshotSpec& spec,
                           const boost::json::value& json);
