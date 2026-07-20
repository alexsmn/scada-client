#pragma once

struct ScreenshotSpec;

// Renders the KPI severity tile strip — the context bar's alarm summary
// (backlog 2.3) — and saves it under `GetOutputDir() / spec.filename`.
//
// Standalone and node-service-free: the strip's value is how the counts read,
// so the capture seeds a live alarm picture (a critical, warnings, and a larger
// unacknowledged backlog) rather than depending on the fixture happening to
// hold unacknowledged alarms — which it does not, so the in-window tiles would
// all render zero.
void SaveSeverityTilesScreenshot(const ScreenshotSpec& spec);
