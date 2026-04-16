#pragma once

#include <filesystem>

// Returns the directory where captured PNGs should land.
// The caller must provide `--out <dir>`.
std::filesystem::path GetOutputDir();
