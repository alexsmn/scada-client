#pragma once

#include <boost/json/value.hpp>

#include <filesystem>
#include <string>
#include <vector>

// Configuration for a single main-window view capture. Matches one row
// in the `screenshots:` array of `screenshot_data.json`.
struct ScreenshotSpec {
  std::string window_type;
  std::string filename;
  std::string path;
  int width = 800;
  int height = 600;
};

// Configuration for a single modal-dialog capture. `kind` is the
// dispatch key in `dialog_capture.cpp`.
struct DialogSpec {
  std::string kind;
  std::string filename;
  int width = 0;
  int height = 0;
};

// Screenshot-generator fixture, loaded once from `screenshot_data.json`.
// Holds the parsed JSON tree plus the lightweight spec vectors; the
// per-capture files consult one or the other.
struct ScreenshotConfig {
  boost::json::value json;
  std::vector<ScreenshotSpec> screenshots;
  std::vector<DialogSpec> dialogs;

  // Reads `path` and populates the fields. Uses `ASSERT_*` on failure
  // so a bad fixture fails the test suite before any TEST_F runs.
  void Load(const std::filesystem::path& path);
};

// Locates `screenshot_data.json` on disk: next to this file, then the
// current working directory. Falls back to a bare filename if neither
// exists.
std::filesystem::path GetDataFilePath();
