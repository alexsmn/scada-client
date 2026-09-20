#include "modus/modus_util.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

namespace {

using std::filesystem::path;

// `IsModus2` and `DocumentKindFor` had their own cases here until backlog 491
// retired both. They pinned a `DocumentKind` the reader discarded -- including
// the `.xsde`-with-the-flag-off case, which asserted the value whose only
// correct use was to be ignored.

TEST(IsModusFilePathTest, AcceptsBothModusExtensionsRegardlessOfCase) {
  EXPECT_TRUE(IsModusFilePath("scheme.sde"));
  EXPECT_TRUE(IsModusFilePath("scheme.xsde"));
  EXPECT_TRUE(IsModusFilePath("SCHEME.SDE"));
  EXPECT_TRUE(IsModusFilePath("Scheme.XsDe"));
}

TEST(IsModusFilePathTest, RejectsAnythingElse) {
  EXPECT_FALSE(IsModusFilePath("drawing.vds"));
  EXPECT_FALSE(IsModusFilePath("scheme"));
  EXPECT_FALSE(IsModusFilePath("scheme.sde.bak"));
  EXPECT_FALSE(IsModusFilePath(""));
}

TEST(MakeModusFilePathTest,
     ARelativeHyperlinkResolvesAgainstTheDisplaysDirectory) {
  const auto result = MakeModusFilePath("target.sde", "schemes/current.sde");

  ASSERT_TRUE(result.has_value());
  EXPECT_EQ(*result, path{"schemes/target.sde"});
}

TEST(MakeModusFilePathTest, ADirectoryTraversalIsNormalisedAndKept) {
  const auto result =
      MakeModusFilePath("../other/target.sde", "schemes/current.sde");

  ASSERT_TRUE(result.has_value());
  EXPECT_EQ(*result, path{"other/target.sde"});
}

// The traversal above stays inside the public directory. One that escapes it is
// not addressable as a public path, and opening the same-named file that
// happens to sit in the public root — which is what the old collapse did — is
// the behaviour this rejects.
TEST(MakeModusFilePathTest, AHyperlinkEscapingThePublicDirectoryIsRejected) {
  EXPECT_FALSE(
      MakeModusFilePath("../../secrets/target.sde", "schemes/current.sde"));

  EXPECT_FALSE(MakeModusFilePath("../target.sde", "current.sde"));
}

TEST(MakeModusFilePathTest, ASiblingHyperlinkKeepsAMultiLevelDirectory) {
  const auto result = MakeModusFilePath("sub/target.sde", "a/b/current.sde");

  ASSERT_TRUE(result.has_value());
  EXPECT_EQ(*result, path{"a/b/sub/target.sde"});
}

}  // namespace
