#include "screenshot_output.h"

#include "screenshot_options.h"

std::filesystem::path GetOutputDir() {
  return GetScreenshotOptions().output_dir.lexically_normal();
}
