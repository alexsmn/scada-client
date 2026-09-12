#include "modus/modus_util.h"

#include "profile/profile.h"
#include "profile/window_definition.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

namespace {

using std::filesystem::path;

// Task 483 settled the question this file used to record: the version-2
// renderer is a live product direction, so these functions are wired up rather
// than deleted. `IsModus2` is now reached in production through
// `DocumentKindFor`, which `ModusController::Init` calls to choose the document
// kind it opens the runtime with, and the two path defects the previous
// revision pinned are fixed below rather than described.

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

class IsModus2Test : public testing::Test {
 protected:
  Profile profile_;
};

// The profile flag is only ever consulted for an `.xsde` document: a `.sde` is
// the legacy format and has no version-2 rendering, so the flag is forced off.
TEST_F(IsModus2Test, TheProfileFlagAppliesOnlyToXsdeDocuments) {
  profile_.modus.modus2 = true;

  WindowDefinition xsde;
  xsde.path = "scheme.xsde";
  EXPECT_TRUE(IsModus2(xsde, profile_));

  WindowDefinition sde;
  sde.path = "scheme.sde";
  EXPECT_FALSE(IsModus2(sde, profile_));
}

TEST_F(IsModus2Test, TheProfileFlagOffMeansVersionOneEvenForXsde) {
  profile_.modus.modus2 = false;

  WindowDefinition definition;
  definition.path = "scheme.xsde";

  EXPECT_FALSE(IsModus2(definition, profile_));
}

// A document that records its own Options/version overrides the profile in both
// directions; version 0 means "not recorded" and defers to the profile.
TEST_F(IsModus2Test, AnExplicitDocumentVersionOverridesTheProfile) {
  profile_.modus.modus2 = false;

  WindowDefinition definition;
  definition.path = "scheme.xsde";
  definition.AddItem("Options").SetInt("version", 2);

  EXPECT_TRUE(IsModus2(definition, profile_));
}

TEST_F(IsModus2Test, AVersionBelowTwoOverridesAProfileThatSaysOtherwise) {
  profile_.modus.modus2 = true;

  WindowDefinition definition;
  definition.path = "scheme.xsde";
  definition.AddItem("Options").SetInt("version", 1);

  EXPECT_FALSE(IsModus2(definition, profile_));
}

TEST_F(IsModus2Test, VersionZeroMeansUnrecordedAndDefersToTheProfile) {
  profile_.modus.modus2 = true;

  WindowDefinition definition;
  definition.path = "scheme.xsde";
  definition.AddItem("Options").SetInt("version", 0);

  EXPECT_TRUE(IsModus2(definition, profile_));
}

// The extension check runs last and unconditionally, so it overrides even an
// explicit document version. A `.sde` recorded as version 2 is still version 1.
TEST_F(IsModus2Test, TheExtensionOverridesEvenAnExplicitVersion) {
  profile_.modus.modus2 = true;

  WindowDefinition definition;
  definition.path = "scheme.sde";
  definition.AddItem("Options").SetInt("version", 2);

  EXPECT_FALSE(IsModus2(definition, profile_));
}

// `MakeModusFilePath` resolves a hyperlink against the current display's
// directory and returns a public-relative path. Until task 483 it passed the
// result through `FullFilePathToPublic`, which was `path.filename()`, so every
// case below collapsed to a bare filename and the directory logic above it
// could never be observed — a hyperlink to `../other/scheme.sde` opened
// whichever `scheme.sde` the public root happened to hold.
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

// `DocumentKindFor` is the seam that carries the version-2 choice to the
// renderer; `ModusController::Init` passes its result to the runtime instead of
// DocumentKind::kAuto. It follows `IsModus2` exactly, which is
// what makes the «Use Modus runtime renderer» command observable.
class DocumentKindForTest : public testing::Test {
 public:
  Profile profile_;
};

TEST_F(DocumentKindForTest, AnXsdeDocumentFollowsTheProfileFlag) {
  WindowDefinition definition;
  definition.path = "scheme.xsde";

  profile_.modus.modus2 = true;
  EXPECT_EQ(DocumentKindFor(definition, profile_),
            scada::display::view::DocumentKind::kXsde);

  profile_.modus.modus2 = false;
  EXPECT_EQ(DocumentKindFor(definition, profile_),
            scada::display::view::DocumentKind::kSde);
}

TEST_F(DocumentKindForTest, AnSdeDocumentIsVersionOneWhateverTheProfileSays) {
  WindowDefinition definition;
  definition.path = "scheme.sde";

  profile_.modus.modus2 = true;
  EXPECT_EQ(DocumentKindFor(definition, profile_),
            scada::display::view::DocumentKind::kSde);
}

}  // namespace
