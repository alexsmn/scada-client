#include "screenshot_output.h"

#include <gtest/gtest.h>

namespace {

const std::filesystem::path kDir{"/out"};

// The default appearance keeps the bare name from `screenshot_data.json`.
// Every consumer of the gallery -- the image manifest, the scada-docs publish
// subset, check_screenshots.py -- refers to captures by that name, so the
// theme axis had to be added without moving any of them.
TEST(ScreenshotOutputTest, TheDefaultThemeKeepsTheBareFilename) {
  EXPECT_EQ(ThemedOutputPath(kDir, "devices.png", "dark"),
            kDir / "devices.png");
}

// Anything else renders beside it rather than over it. Before this, both runs
// wrote `GetOutputDir() / spec.filename` and the second silently replaced the
// first -- which is why the gallery had only ever held one appearance.
TEST(ScreenshotOutputTest, AnotherThemeRendersBesideItWithASuffix) {
  EXPECT_EQ(ThemedOutputPath(kDir, "devices.png", "light"),
            kDir / "devices-light.png");
  EXPECT_EQ(ThemedOutputPath(kDir, "devices.png", "hc"),
            kDir / "devices-hc.png");
}

// The suffix goes before the extension, not after the name. `devices.png-light`
// would still be written and would still look right in a directory listing,
// while being invisible to every tool that selects captures by `.png`.
TEST(ScreenshotOutputTest, TheSuffixPrecedesTheExtension) {
  const std::filesystem::path path =
      ThemedOutputPath(kDir, "devices.png", "light");
  EXPECT_EQ(path.extension(), ".png");
  EXPECT_EQ(path.stem(), "devices-light");
}

// A filename with dots of its own keeps all but the last:
// `stem()`/`extension()` split at the final dot, so only the real extension
// moves.
TEST(ScreenshotOutputTest, OnlyTheFinalExtensionIsSplit) {
  EXPECT_EQ(
      ThemedOutputPath(kDir, "menu-parameters-elements.copy.png", "light"),
      kDir / "menu-parameters-elements.copy-light.png");
}

// An empty theme is treated as the default rather than producing
// `devices-.png`. Nothing passes one today -- the option defaults to "dark" --
// but the failure mode if something did is a file whose name no manifest row
// would ever match.
TEST(ScreenshotOutputTest, AnEmptyThemeIsTheDefault) {
  EXPECT_EQ(ThemedOutputPath(kDir, "devices.png", ""), kDir / "devices.png");
}

}  // namespace
