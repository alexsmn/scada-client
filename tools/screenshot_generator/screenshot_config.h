#pragma once

#include <boost/json/value.hpp>

#include "scada/node_id.h"

#include <filesystem>
#include <string>
#include <vector>

// Configuration for a single main-window view capture. Matches one row
// in the `screenshots:` array of `screenshot_data.json`.
struct ScreenshotSpec {
  std::string window_type;
  std::string filename;
  std::string path;
  // Optional multiple item paths/formulas. Views that hold a list of data
  // items (Table, Watch) seed one row per entry so the capture shows a
  // populated grid instead of an empty view.
  std::vector<std::string> paths;
  int width = 800;
  int height = 600;
  // Minimum number of grid rows the rendered window must show. 0 disables
  // the check. Set it for grid-backed windows (node tables, transmission)
  // so a data-path regression fails the capture instead of silently saving
  // an empty frame.
  int min_rows = 0;
  // Exact grid row count ("rows" in the JSON): additionally catches rows
  // leaking IN from outside the captured scope, not just an empty grid.
  int exact_rows = 0;
  // Optional: objectName of a child button to click before grabbing (e.g. a
  // subtab), so a capture can show a non-default tab of a multi-tab view.
  std::string click_object;
  // Expand every row of a tree-backed window before grabbing. A collapsed
  // tree captures its folders and hides everything the capture is about.
  bool expand = false;
};

// Configuration for a single modal-dialog capture. `kind` is the
// dispatch key in `dialog_capture.cpp`.
struct DialogSpec {
  std::string kind;
  std::string filename;
  int width = 0;
  int height = 0;
  // Renders only under --theme ("themed_only" in the JSON). Lets one dialog
  // kind have both a legacy and a reshell capture without either run
  // overwriting the other's image.
  bool themed_only = false;
};

// Screenshot-generator fixture, loaded once from `screenshot_data.json`.
// Holds the parsed JSON tree plus the lightweight spec vectors; the
// per-capture files consult one or the other.
struct ScreenshotConfig {
  boost::json::value json;
  std::vector<ScreenshotSpec> screenshots;
  std::vector<DialogSpec> dialogs;
  scada::NodeId dialog_analog_node_id;

  // Reads `path` and populates the fields. Uses `ASSERT_*` on failure
  // so a bad fixture fails the test suite before any TEST_F runs.
  void Load(const std::filesystem::path& path);
};

// Returns true when `filename` is tagged `auto-*` in the image manifest and
// should therefore be rendered by the screenshot generator.
bool IsAutoManagedImageFilename(std::string_view filename);

// Locates `screenshot_data.json` on disk: next to this file, then the
// current working directory. Falls back to a bare filename if neither
// exists.
std::filesystem::path GetDataFilePath();
