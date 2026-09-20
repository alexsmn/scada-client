#include "services/display_selection_registry.h"

#include <gtest/gtest.h>

namespace {

// Two stand-ins for display views. The registry only ever compares these, so
// any distinct addresses do; using ints rather than real views keeps the test
// free of Qt. Distinct *values* as well, so no constant merging can make the
// two owners compare equal.
const int kViewA = 1;
const int kViewB = 2;

TEST(DisplaySelectionRegistryTest, StartsEmpty) {
  DisplaySelectionRegistry registry;
  EXPECT_TRUE(registry.label().empty());
}

TEST(DisplaySelectionRegistryTest, PublishesAndNotifies) {
  DisplaySelectionRegistry registry;
  int changes = 0;
  auto connection = registry.SubscribeChanged([&changes] { ++changes; });

  registry.SetSelection(&kViewA, u"Q1");

  EXPECT_EQ(registry.label(), u"Q1");
  EXPECT_EQ(changes, 1);
}

// The status strip repaints on every notification, so re-selecting the shape
// already shown must not produce one.
TEST(DisplaySelectionRegistryTest, RepublishingTheSameLabelIsSilent) {
  DisplaySelectionRegistry registry;
  registry.SetSelection(&kViewA, u"Q1");

  int changes = 0;
  auto connection = registry.SubscribeChanged([&changes] { ++changes; });

  registry.SetSelection(&kViewA, u"Q1");

  EXPECT_EQ(registry.label(), u"Q1");
  EXPECT_EQ(changes, 0);
}

// A click on bare page clears, and an empty label is how that arrives.
TEST(DisplaySelectionRegistryTest, AnEmptyLabelClears) {
  DisplaySelectionRegistry registry;
  registry.SetSelection(&kViewA, u"Q1");

  registry.SetSelection(&kViewA, u"");

  EXPECT_TRUE(registry.label().empty());
}

TEST(DisplaySelectionRegistryTest, LastSelectionWins) {
  DisplaySelectionRegistry registry;

  registry.SetSelection(&kViewA, u"Q1");
  registry.SetSelection(&kViewB, u"T1");

  EXPECT_EQ(registry.label(), u"T1");
}

// The reason ClearSelection takes an owner at all: a background display's
// teardown must not blank the readout for the one in front of the operator.
TEST(DisplaySelectionRegistryTest, ClearingFromAStaleOwnerIsIgnored) {
  DisplaySelectionRegistry registry;
  registry.SetSelection(&kViewA, u"Q1");
  registry.SetSelection(&kViewB, u"T1");

  int changes = 0;
  auto connection = registry.SubscribeChanged([&changes] { ++changes; });

  registry.ClearSelection(&kViewA);

  EXPECT_EQ(registry.label(), u"T1");
  EXPECT_EQ(changes, 0);
}

TEST(DisplaySelectionRegistryTest, ClearingFromTheCurrentOwnerClears) {
  DisplaySelectionRegistry registry;
  registry.SetSelection(&kViewA, u"Q1");

  int changes = 0;
  auto connection = registry.SubscribeChanged([&changes] { ++changes; });

  registry.ClearSelection(&kViewA);

  EXPECT_TRUE(registry.label().empty());
  EXPECT_EQ(changes, 1);
}

// A view clears on both the bare-page click and its own teardown, so the
// second call must not notify again.
TEST(DisplaySelectionRegistryTest, ClearingTwiceNotifiesOnce) {
  DisplaySelectionRegistry registry;
  registry.SetSelection(&kViewA, u"Q1");

  int changes = 0;
  auto connection = registry.SubscribeChanged([&changes] { ++changes; });

  registry.ClearSelection(&kViewA);
  registry.ClearSelection(&kViewA);

  EXPECT_TRUE(registry.label().empty());
  EXPECT_EQ(changes, 1);
}

}  // namespace
