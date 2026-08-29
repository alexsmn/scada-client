#pragma once

#include <filesystem>
#include <span>
#include <string>
#include <string_view>
#include <unordered_set>

struct ScreenshotOptions {
  std::filesystem::path output_dir;
  std::filesystem::path image_manifest;
  // Fixture to render from (`--data`). Empty means the search in
  // `GetDataFilePath()`, which resolves the source tree's own
  // `screenshot_data.json`. Naming one lets a run drive an edited copy without
  // writing to the fixture other sessions share.
  std::filesystem::path data_file;
  std::unordered_set<std::string> only_filenames;
  // Optional design-token theme to render captures under ("dark"|"light"|"hc").
  // Empty means the legacy Fusion look. Used to validate the UX reshell against
  // real Qt widgets (see docs/client/ux/ and client/CLAUDE.md).
  std::string theme;
};

// Parses `args` (the command line without argv[0]) into an options struct.
//
// Throws `std::runtime_error` naming every option it does not recognise. That
// is deliberate and is the whole point of the function: the parser accepted and
// silently discarded unknown flags until 2026-08-29, so `--data=<copy>` — a
// flag that did not exist — looked like it worked and rendered the tracked
// fixture instead, and the run that was meant to prove something proved the
// opposite (backlog 631). Throwing matches what `--out` already did when
// missing, via program_options' own `required()`.
//
// `--gtest_*` is passed through untouched: this binary's `main` comes from
// gtest, which parses those itself, and they arrive here because the args are
// read from the process rather than from the argv gtest has already stripped.
ScreenshotOptions ParseScreenshotOptions(std::span<const std::string> args);

void InitScreenshotOptions();
const ScreenshotOptions& GetScreenshotOptions();
bool ShouldCaptureScreenshot(std::string_view filename);
