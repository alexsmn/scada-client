#pragma once

#include "base/any_executor.h"

#include <boost/json/value.hpp>

#include <string_view>

class MainWindow;
class NodeService;
class TimedDataService;
struct ScreenshotSpec;

namespace scada {
class AttributeService;
}

namespace scada::screenshot_generator {

// Everything a standalone capture can ask for.
//
// A standalone capture builds its own fixture instead of grabbing an opened
// view, so the dispatch table holds one signature instead of twenty; this
// bundle carries the union of what the capture functions take, and most
// entries use two or three of its members.
struct StandaloneCaptureContext {
  const ScreenshotSpec& spec;
  MainWindow& main_window;
  NodeService& node_service;
  TimedDataService& timed_data_service;
  scada::AttributeService& authenticated_attribute_service;
  const boost::json::value& json;
  AnyExecutor executor;
};

// One standalone capture: the `capture` key a fixture spec selects it by, and
// the function that renders it. A null `save` means the spec is rendered by
// another TEST_F, so the sweep skips it without counting it as captured.
struct StandaloneCapture {
  std::string_view key;
  void (*save)(const StandaloneCaptureContext&);
};

// Resolves a spec's `capture` key. Null means the key is not one this generator
// knows, which is a fixture error rather than a skip.
const StandaloneCapture* FindStandaloneCapture(std::string_view key);

}  // namespace scada::screenshot_generator
