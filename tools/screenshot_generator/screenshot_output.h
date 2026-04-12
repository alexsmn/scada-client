#pragma once

#include <filesystem>

// Returns the directory where captured PNGs should land.
//
// Resolution order, computed once on first call and cached:
//   1. `SCREENSHOT_OUT_DIR` environment variable.
//   2. `<cwd>/screenshots` — the original default.
//
// Lets a docs-side script point the generator at `scada-docs/img/`
// without editing the source. A `--out` CLI flag isn't viable because
// `AppEnvironment` constructs QApplication with a zeroed argv, so the
// real args aren't reachable via `QCoreApplication::arguments()`.
std::filesystem::path GetOutputDir();
