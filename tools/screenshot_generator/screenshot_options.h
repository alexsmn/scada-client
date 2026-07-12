#pragma once

#include <filesystem>
#include <string>
#include <string_view>
#include <unordered_set>

struct ScreenshotOptions {
  std::filesystem::path output_dir;
  std::filesystem::path image_manifest;
  std::unordered_set<std::string> only_filenames;
  // Optional design-token theme to render captures under ("dark"|"light"|"hc").
  // Empty means the legacy Fusion look. Used to validate the UX reshell against
  // real Qt widgets (see client/docs/ux/ and client/CLAUDE.md).
  std::string theme;
};

void InitScreenshotOptions();
const ScreenshotOptions& GetScreenshotOptions();
bool ShouldCaptureScreenshot(std::string_view filename);
