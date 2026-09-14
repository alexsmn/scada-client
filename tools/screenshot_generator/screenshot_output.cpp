#include "screenshot_output.h"

#include "screenshot_options.h"

#include <string>

namespace {

// The theme whose renders carry the bare filename. Every other theme is
// suffixed, so one directory holds them all without collision.
constexpr std::string_view kUnsuffixedTheme = "dark";

}  // namespace

std::filesystem::path GetOutputDir() {
  return GetScreenshotOptions().output_dir.lexically_normal();
}

std::filesystem::path ThemedOutputPath(const std::filesystem::path& dir,
                                       std::string_view filename,
                                       std::string_view theme) {
  const std::filesystem::path name{filename};
  if (theme == kUnsuffixedTheme || theme.empty())
    return dir / name;

  // Insert before the extension rather than appending, so the result is still
  // a .png to every consumer that keys on the suffix -- the image manifest,
  // check_screenshots.py and the publish step all do.
  std::filesystem::path suffixed = name.stem();
  suffixed += "-";
  suffixed += theme;
  suffixed += name.extension();
  return dir / suffixed;
}

std::filesystem::path OutputPathFor(std::string_view filename) {
  return ThemedOutputPath(GetOutputDir(), filename,
                          GetScreenshotOptions().theme);
}
