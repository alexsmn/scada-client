#pragma once

#include "base/any_executor.h"
#include <boost/json/value.hpp>

class NodeService;
class TimedDataService;
struct ScreenshotSpec;
class WindowDefinition;

// Returns a `WindowDefinition` of type "Graph" built from the
// `graph` section of the screenshot-generator JSON. Used inside the
// page/profile path so the MainWindow opens a graph view with the
// fixture's panes, lines and time scale.
WindowDefinition MakeGraphDefinition(const boost::json::value& json);

// Renders a standalone `MetrixGraph` widget populated from JSON timed
// data and saves it under `OutputPathFor(spec.filename)`. This
// bypasses the hidden main-window layout issue: hidden `QSplitter`
// children skip relayout, so we create a fresh graph as a top-level.
void SaveGraphScreenshot(const ScreenshotSpec& spec,
                         AnyExecutor executor,
                         NodeService& node_service,
                         TimedDataService& timed_data_service,
                         const boost::json::value& json);

// Renders the Inspector as a chart selection fills it: builds the fixture
// graph, routes its first series through a SelectionModel into the shell
// Inspector — element card plus the plotted-series section — and saves the
// panel under `OutputPathFor(spec.filename)`. Reshell-only chrome, so the
// caller should apply a `--theme`. The Graph tab carried an inspector of its
// own until 2026-08-30; the series rows moved into this one, which is the
// single right-hand column the trend screens draw.
void SaveSeriesInspectorScreenshot(const ScreenshotSpec& spec,
                                   AnyExecutor executor,
                                   NodeService& node_service,
                                   TimedDataService& timed_data_service,
                                   const boost::json::value& json);
