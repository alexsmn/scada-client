#include "modules/transmission/transmission_address.h"

#include <gtest/gtest.h>

#include <vector>

namespace {

TEST(NextFreeTransmissionAddressTest, AnEmptyTableStartsAtTheBottomOfTheRange) {
  EXPECT_EQ(NextFreeTransmissionAddress({}, {.min = 1, .max = 255}), 1);
  EXPECT_EQ(NextFreeTransmissionAddress({}, {.min = 40001, .max = 50000}),
            40001);
}

// Max+1 rather than the lowest gap: a gap in an agreed address map may be a
// reserved spare, so it is not filled while there is room above.
TEST(NextFreeTransmissionAddressTest, TakesTheHighestPlusOneNotTheFirstGap) {
  const std::vector<int> used{1, 2, 5, 9};
  EXPECT_EQ(NextFreeTransmissionAddress(used, {.min = 1, .max = 255}), 10);
}

TEST(NextFreeTransmissionAddressTest, FallsBackToTheLowestGapAtTheTop) {
  const std::vector<int> used{1, 2, 4, 255};
  EXPECT_EQ(NextFreeTransmissionAddress(used, {.min = 1, .max = 255}), 3);
}

TEST(NextFreeTransmissionAddressTest, IsNullWhenTheRangeIsFull) {
  const std::vector<int> used{1, 2, 3};
  EXPECT_EQ(NextFreeTransmissionAddress(used, {.min = 1, .max = 3}),
            std::nullopt);
}

// An address outside the range (a stray value typed into another register
// table) neither pushes max+1 out of the range nor occupies it.
TEST(NextFreeTransmissionAddressTest, IgnoresAddressesOutsideTheRange) {
  const std::vector<int> used{40001, 40002, 70000, 5};
  EXPECT_EQ(NextFreeTransmissionAddress(used, {.min = 40001, .max = 50000}),
            40003);
}

TEST(NextFreeTransmissionAddressTest, DuplicatesDoNotConfuseIt) {
  const std::vector<int> used{1, 1, 1};
  EXPECT_EQ(NextFreeTransmissionAddress(used, {}), 2);
}

TEST(DuplicateTransmissionAddressesTest, ReportsEachRepeatedAddressOnce) {
  const std::vector<int> addresses{1, 2, 1, 3, 2, 1};
  EXPECT_EQ(DuplicateTransmissionAddresses(addresses), (std::set<int>{1, 2}));
}

TEST(DuplicateTransmissionAddressesTest, DistinctAddressesHaveNone) {
  const std::vector<int> addresses{1, 2, 3};
  EXPECT_TRUE(DuplicateTransmissionAddresses(addresses).empty());
}

}  // namespace
