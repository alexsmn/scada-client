#include "parameter_form/parameter_staging.h"

#include <gtest/gtest.h>

namespace {

// Two distinct opaque group pointers for keys.
int group_a = 0;
int group_b = 0;

ParameterStaging::Key KeyA(int index) {
  return {&group_a, index};
}
ParameterStaging::Key KeyB(int index) {
  return {&group_b, index};
}

TEST(ParameterStagingTest, StartsCleanAndEmpty) {
  ParameterStaging staging;
  EXPECT_FALSE(staging.dirty());
  EXPECT_EQ(staging.size(), 0u);
  EXPECT_EQ(staging.Get(KeyA(0)), nullptr);
}

TEST(ParameterStagingTest, SetStagesAValueAndMarksDirty) {
  ParameterStaging staging;
  staging.Set(KeyA(0), u"2404");

  EXPECT_TRUE(staging.dirty());
  EXPECT_EQ(staging.size(), 1u);
  ASSERT_NE(staging.Get(KeyA(0)), nullptr);
  EXPECT_EQ(*staging.Get(KeyA(0)), u"2404");
}

TEST(ParameterStagingTest, KeysAreDistinctPerGroupAndIndex) {
  ParameterStaging staging;
  staging.Set(KeyA(0), u"a0");
  staging.Set(KeyA(1), u"a1");
  staging.Set(KeyB(0), u"b0");

  EXPECT_EQ(staging.size(), 3u);
  EXPECT_EQ(*staging.Get(KeyA(0)), u"a0");
  EXPECT_EQ(*staging.Get(KeyA(1)), u"a1");
  EXPECT_EQ(*staging.Get(KeyB(0)), u"b0");
}

TEST(ParameterStagingTest, SetOverwritesPriorValue) {
  ParameterStaging staging;
  staging.Set(KeyA(0), u"first");
  staging.Set(KeyA(0), u"second");

  EXPECT_EQ(staging.size(), 1u);
  EXPECT_EQ(*staging.Get(KeyA(0)), u"second");
}

TEST(ParameterStagingTest, RemoveDropsTheEditAndCanClearDirty) {
  ParameterStaging staging;
  staging.Set(KeyA(0), u"x");
  staging.Remove(KeyA(0));

  EXPECT_FALSE(staging.dirty());
  EXPECT_EQ(staging.Get(KeyA(0)), nullptr);
}

TEST(ParameterStagingTest, ClearDiscardsEverything) {
  ParameterStaging staging;
  staging.Set(KeyA(0), u"x");
  staging.Set(KeyB(1), u"y");
  staging.Clear();

  EXPECT_FALSE(staging.dirty());
  EXPECT_EQ(staging.size(), 0u);
}

TEST(ParameterStagingTest, EditsExposesStagedPairsForReplay) {
  ParameterStaging staging;
  staging.Set(KeyA(2), u"v");
  ASSERT_EQ(staging.edits().size(), 1u);
  const auto& [key, value] = *staging.edits().begin();
  EXPECT_EQ(key.first, &group_a);
  EXPECT_EQ(key.second, 2);
  EXPECT_EQ(value, u"v");
}

}  // namespace
