#pragma once

struct ScreenshotSpec;

// Renders the reshell Inspector panel standalone (the right-hand selection
// inspector of table-watch.html / substation-display.html), filled with a
// representative Table expression-row selection — a live computed value with
// no backing node, the shape the panel supports since UX 2.8.
void SaveInspectorScreenshot(const ScreenshotSpec& spec);

// Renders the Inspector's event (alarm) card — the per-event inspector of a
// journal-row selection (UX 2.2): severity band pill, message, event time,
// acknowledgement state and the Acknowledge action.
void SaveInspectorEventScreenshot(const ScreenshotSpec& spec);
