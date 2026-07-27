#pragma once

struct ScreenshotSpec;

// Renders the device log's frame-decode pane — the right-hand inspector of
// docs/ui-mockups/screens/device-protocol-trace.html — over a fixture
// IEC 60870-5-104 APDU, and saves it under `GetOutputDir() / spec.filename`.
//
// Standalone, like SaveSeverityTilesScreenshot: the pane is built directly and
// fed one frame. Capturing it through WatchView would mean assembling a full
// ControllerContext (fifteen services), and would show the same widget.
void SaveFrameDecodeScreenshot(const ScreenshotSpec& spec);
