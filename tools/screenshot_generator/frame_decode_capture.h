#pragma once

struct ScreenshotSpec;

// Renders the device log's frame-decode pane — the right-hand inspector of
// docs/product/ui-mockups/screens/device-protocol-trace.html — over a fixture
// IEC 60870-5-104 APDU, and saves it under `OutputPathFor(spec.filename)`.
//
// Standalone, like SaveSeverityTilesScreenshot: the pane is built directly and
// fed one frame. Capturing it through WatchView would mean assembling a full
// ControllerContext (fifteen services), and would show the same widget.
void SaveFrameDecodeScreenshot(const ScreenshotSpec& spec);

// Renders the device-log filter bar (frame kind, errors-only, free text) on
// its own. The bar is a strip of stock widgets, so capturing it alone shows
// what a full-view capture would — without needing a live controller.
void SaveWatchFilterBarScreenshot(const ScreenshotSpec& spec);
