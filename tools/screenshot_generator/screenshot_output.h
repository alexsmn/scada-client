#pragma once

#include <filesystem>

// Returns the directory where captured PNGs should land.
//
// The caller must provide `SCREENSHOT_OUT_DIR`. A `--out` CLI flag
// isn't viable because `AppEnvironment` constructs QApplication with a
// zeroed argv, so the real args aren't reachable via
// `QCoreApplication::arguments()`.
std::filesystem::path GetOutputDir();
