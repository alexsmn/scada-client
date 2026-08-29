#include "screenshot_options.h"

#include <gtest/gtest.h>

#include <stdexcept>
#include <string>
#include <vector>

namespace {

// The generator accepted any flag and silently discarded the ones it did not
// know, so `--data=<copy>` — which did not exist as an option — looked like it
// worked and rendered the tracked fixture instead (backlog 631). These cases
// fail against that parser: every unknown-option case passed it.

ScreenshotOptions Parse(std::vector<std::string> args) {
  return ParseScreenshotOptions(args);
}

TEST(ScreenshotOptionsTest, RejectsAnUnknownOption) {
  EXPECT_THROW(Parse({"--out=/tmp/x", "--this-flag-does-not-exist=1"}),
               std::runtime_error);
}

TEST(ScreenshotOptionsTest, NamesTheUnknownOptionItRejected) {
  try {
    Parse({"--out=/tmp/x", "--typo-flag=1"});
    FAIL() << "expected an unknown option to be rejected";
  } catch (const std::runtime_error& error) {
    EXPECT_NE(std::string(error.what()).find("--typo-flag"), std::string::npos)
        << "diagnostic must name the offending option: " << error.what();
  }
}

TEST(ScreenshotOptionsTest, RejectsAMisspeltKnownOption) {
  // The shape that actually bites: one character off a real flag.
  EXPECT_THROW(Parse({"--out=/tmp/x", "--imagemanifest=/tmp/m.json"}),
               std::runtime_error);
}

TEST(ScreenshotOptionsTest, PassesGtestFlagsThrough) {
  // This binary's main() comes from gtest, which parses these itself; they
  // reach the parser because the args are read from the process rather than
  // from the argv gtest has already stripped.
  ScreenshotOptions options;
  ASSERT_NO_THROW(
      options = Parse({"--gtest_filter=Foo.Bar",
                       "--gtest_also_run_disabled_tests", "--out=/tmp/x"}));
  EXPECT_EQ(options.output_dir, "/tmp/x");
}

TEST(ScreenshotOptionsTest, ReadsTheFixturePathFromData) {
  const ScreenshotOptions options =
      Parse({"--out=/tmp/x", "--data=/tmp/fixture.json"});
  EXPECT_EQ(options.data_file, "/tmp/fixture.json");
}

TEST(ScreenshotOptionsTest, LeavesTheFixturePathEmptyWithoutData) {
  // Empty is what makes GetDataFilePath fall back to its own search.
  EXPECT_TRUE(Parse({"--out=/tmp/x"}).data_file.empty());
}

TEST(ScreenshotOptionsTest, StillParsesEveryKnownOption) {
  const ScreenshotOptions options = Parse({
      "--out=/tmp/out",
      "--image-manifest=/tmp/manifest.json",
      "--data=/tmp/fixture.json",
      "--only=a.png,b.png",
      "--theme=dark",
  });
  EXPECT_EQ(options.output_dir, "/tmp/out");
  EXPECT_EQ(options.image_manifest, "/tmp/manifest.json");
  EXPECT_EQ(options.data_file, "/tmp/fixture.json");
  EXPECT_EQ(options.theme, "dark");
  EXPECT_EQ(options.only_filenames,
            (std::unordered_set<std::string>{"a.png", "b.png"}));
}

TEST(ScreenshotOptionsTest, StillRequiresAnOutputDirectory) {
  EXPECT_THROW(Parse({"--theme=dark"}), std::exception);
}

}  // namespace
