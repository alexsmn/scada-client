#pragma once

struct ScreenshotSpec;

// Renders the reshelled bulk-create wizard's Naming/Addressing preview — the
// center of bulk-create.html — and saves it under
// `GetOutputDir() / spec.filename`.
//
// Standalone and node-service-free: the panel's value is the pure pattern
// engine + live preview, so the capture seeds a demo `{n}`-token template and a
// one-entry existing-NodeId set (so one row renders as a conflict), builds a
// fresh BulkCreatePreviewPanel, and grabs it.
void SaveBulkCreateScreenshot(const ScreenshotSpec& spec);
