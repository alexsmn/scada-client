#pragma once

#include "base/any_executor.h"

struct ScreenshotSpec;
class MainWindow;
class NodeService;
class TimedDataService;

// Renders the device Metrics sheet — the window the operator reaches with
// *Метрики* on a device's context menu (client.md, "Статусы устройств") — for
// the fixture device named by `spec.path`, and saves it under
// `OutputPathFor(spec.filename)`.
//
// Unlike the other standalone captures this one goes through the real window
// pipeline rather than building a widget by hand: the metrics view is not a
// window type of its own but a `CusTable` (kSheetWindowInfo) whose cells
// DeviceMetricsModule computes from the device's type-definition data
// variables. Building the WindowDefinition with the module's own
// MakeDeviceMetricsWindowDefinitionAsync and then opening it is what keeps the
// capture honest — a change to the collected variable set, the header layout,
// or the `=NodeId` cell formulas shows up in the image.
//
// It cannot ride the profile page like an ordinary spec: the cells are derived
// from resolved NodeRefs, which do not exist until the node service is up, and
// MakeScreenshotPage builds the page from JSON before that.
void SaveDeviceMetricsScreenshot(const ScreenshotSpec& spec,
                                 MainWindow& main_window,
                                 NodeService& node_service,
                                 TimedDataService& timed_data_service,
                                 AnyExecutor executor);
