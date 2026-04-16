#pragma once

#include <filesystem>
#include <string>
#include <string_view>
#include <unordered_set>

struct ScreenshotOptions {
  std::filesystem::path output_dir;
  std::filesystem::path image_manifest;
  std::unordered_set<std::string> only_filenames;
};

void InitScreenshotOptions();
const ScreenshotOptions& GetScreenshotOptions();
bool ShouldCaptureScreenshot(std::string_view filename);
