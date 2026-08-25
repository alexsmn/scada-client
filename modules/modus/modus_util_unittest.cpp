#include "modus/modus_util.h"

#include "profile/profile.h"
#include "profile/window_definition.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

namespace {

using std::filesystem::path;

// NOTE (task 479): every function under test here is currently **unreachable**
// from the running client — a tree-wide search on 2026-08-25 found no caller of
// any of them. They are compiled into `client_modus_qt` and called by nothing,
// having been left behind when `224210f46` routed Modus through the VDS runtime
// and `qt/modus_view{,2}.cpp` were dropped from the build. These tests describe
// what the code does today so the decision recorded in task 481 — wire it back
// up or delete it — is taken against measured behaviour rather than a reading
// of the source. Two of the behaviours below are defects, and are marked.

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

// DEFECT (task 481): `MakeModusFilePath` resolves a hyperlink against the
// current display's directory and normalises the result — and then hands it to
// `FullFilePathToPublic`, which is `path.filename()`. Every one of these cases
// therefore collapses to a bare filename, and the directory logic above it can
// never be observed. A hyperlink to `../other/scheme.sde` resolves to
// `scheme.sde`, which reopens the wrong file whenever two directories hold the
// same name. These tests pin the collapse rather than the intent.
TEST(MakeModusFilePathTest, ARelativeHyperlinkCollapsesToItsBareFilename) {
  const auto result = MakeModusFilePath("target.sde", "schemes/current.sde");

  ASSERT_TRUE(result.has_value());
  EXPECT_EQ(*result, path{"target.sde"});
}

TEST(MakeModusFilePathTest, ADirectoryTraversalIsNormalisedThenDiscarded) {
  const auto result =
      MakeModusFilePath("../other/target.sde", "schemes/current.sde");

  ASSERT_TRUE(result.has_value());
  // Intent: "other/target.sde". Actual: the directory is dropped.
  EXPECT_EQ(*result, path{"target.sde"});
}

TEST(MakeModusFilePathTest, AnAbsoluteHyperlinkAlsoCollapses) {
  const auto result =
      MakeModusFilePath("/var/schemes/target.sde", "schemes/current.sde");

  ASSERT_TRUE(result.has_value());
  EXPECT_EQ(*result, path{"target.sde"});
}

}  // namespace
