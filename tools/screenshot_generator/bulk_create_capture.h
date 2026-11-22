#pragma once

#include <boost/json/fwd.hpp>

struct ScreenshotSpec;

// Renders the bulk-create wizard's Naming/Addressing preview — the center of
// bulk-create.html — and saves it under `OutputPathFor(spec.filename)`.
//
// Standalone and node-service-free: the panel's value is the pure pattern
// engine + live preview, so the capture fills it from the fixture's
// `bulk_create` block (`json` is the whole fixture), builds a fresh
// BulkCreatePreviewPanel, and grabs it.
void SaveBulkCreateScreenshot(const ScreenshotSpec& spec,
                              const boost::json::value& json);
