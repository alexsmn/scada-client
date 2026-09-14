#pragma once

#include "base/any_executor.h"

#include <boost/json/value.hpp>

struct ScreenshotSpec;
class NodeService;
class TimedDataService;

// Renders the reshelled Device Diagnostics panel — the right region of
// config-workbench.html — from the fixture and saves it under
// `OutputPathFor(spec.filename)`.
//
// Standalone like SaveSeriesInspectorScreenshot: it makes the fixture device
// (and its diagnostic child variables) resident, builds a fresh
// DeviceDiagnosticsPanel, drives it with the real node/timed-data services via
// ShowDevice, lets the live specs settle, then grabs the widget. This exercises
// the real node-service resolution + live-spec path, so it is a valid
// cross-platform (macOS/Linux) validation of the reshell panel.
void SaveDeviceDiagnosticsScreenshot(const ScreenshotSpec& spec,
                                     NodeService& node_service,
                                     TimedDataService& timed_data_service,
                                     const boost::json::value& json,
                                     AnyExecutor executor);
