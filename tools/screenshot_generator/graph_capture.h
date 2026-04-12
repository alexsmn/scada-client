#pragma once

#include <boost/json/value.hpp>

class TimedDataService;
struct ScreenshotSpec;
class WindowDefinition;

// Returns a `WindowDefinition` of type "Graph" built from the
// `graph` section of the screenshot-generator JSON. Used inside the
// page/profile path so the MainWindow opens a graph view with the
// fixture's panes, lines and time scale.
WindowDefinition MakeGraphDefinition(const boost::json::value& json);

// Renders a standalone `MetrixGraph` widget populated from JSON timed
// data and saves it under `GetOutputDir() / spec.filename`. This
// bypasses the hidden main-window layout issue: hidden `QSplitter`
// children skip relayout, so we create a fresh graph as a top-level.
void SaveGraphScreenshot(const ScreenshotSpec& spec,
                         TimedDataService& timed_data_service,
                         const boost::json::value& json);
