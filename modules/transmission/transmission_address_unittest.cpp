#include "modules/transmission/transmission_address.h"

#include <gtest/gtest.h>

#include <climits>
#include <set>
#include <vector>

namespace {

// One-address rules at `addresses`, as every IEC 60870-5 rule and every MODBUS
// rule without a wide Value format is.
std::vector<TransmissionAddressSpan> Spans(std::vector<int> addresses) {
  std::vector<TransmissionAddressSpan> spans;
  for (int address : addresses)
    spans.push_back({.address = address});
  return spans;
}

TEST(NextFreeTransmissionAddressTest, AnEmptyTableStartsAtTheBottomOfTheRange) {
  EXPECT_EQ(NextFreeTransmissionAddress({}, {.min = 1, .max = 255}), 1);
  EXPECT_EQ(NextFreeTransmissionAddress({}, {.min = 40001, .max = 50000}),
            40001);
}

// Max+1 rather than the lowest gap: a gap in an agreed address map may be a
// reserved spare, so it is not filled while there is room above.
TEST(NextFreeTransmissionAddressTest, TakesTheHighestPlusOneNotTheFirstGap) {
  const std::vector<TransmissionAddressSpan> used = Spans({1, 2, 5, 9});
  EXPECT_EQ(NextFreeTransmissionAddress(used, {.min = 1, .max = 255}), 10);
}

TEST(NextFreeTransmissionAddressTest, FallsBackToTheLowestGapAtTheTop) {
  const std::vector<TransmissionAddressSpan> used = Spans({1, 2, 4, 255});
  EXPECT_EQ(NextFreeTransmissionAddress(used, {.min = 1, .max = 255}), 3);
}

TEST(NextFreeTransmissionAddressTest, IsNullWhenTheRangeIsFull) {
  const std::vector<TransmissionAddressSpan> used = Spans({1, 2, 3});
  EXPECT_EQ(NextFreeTransmissionAddress(used, {.min = 1, .max = 3}),
            std::nullopt);
}

// An address outside the range (a stray value typed into another register
// table) neither pushes max+1 out of the range nor occupies it.
TEST(NextFreeTransmissionAddressTest, IgnoresAddressesOutsideTheRange) {
  const std::vector<TransmissionAddressSpan> used =
      Spans({40001, 40002, 70000, 5});
  EXPECT_EQ(NextFreeTransmissionAddress(used, {.min = 40001, .max = 50000}),
            40003);
}

TEST(NextFreeTransmissionAddressTest, DuplicatesDoNotConfuseIt) {
  const std::vector<TransmissionAddressSpan> used = Spans({1, 1, 1});
  EXPECT_EQ(NextFreeTransmissionAddress(used, {}), 2);
}

// The defect (backlog 1018): a FLOAT rule at 40001 also holds 40002, so the
// next rule used to get 40002 and overwrite half the float on the wire.
TEST(NextFreeTransmissionAddressTest, StepsPastEveryRegisterOfAWideRule) {
  const TransmissionAddressRange holding{.min = 40001, .max = 50000};
  const std::vector<TransmissionAddressSpan> after_float{
      {.address = 40001, .width = 2}};
  EXPECT_EQ(NextFreeTransmissionAddress(after_float, holding), 40003);

  const std::vector<TransmissionAddressSpan> after_double{
      {.address = 40001, .width = 1}, {.address = 40002, .width = 4}};
  EXPECT_EQ(NextFreeTransmissionAddress(after_double, holding), 40006);
}

// The gap fallback must not land inside a wide rule either.
TEST(NextFreeTransmissionAddressTest, TheGapFallbackSkipsAWideRulesRegisters) {
  const std::vector<TransmissionAddressSpan> used{
      {.address = 40001, .width = 4}, {.address = 50000, .width = 1}};
  EXPECT_EQ(NextFreeTransmissionAddress(used, {.min = 40001, .max = 50000}),
            40005);
}

// A wide rule whose last register runs past the top of the range fills the
// top, so the lowest gap is used.
TEST(NextFreeTransmissionAddressTest, AWideRuleAtTheTopFillsIt) {
  const std::vector<TransmissionAddressSpan> used{
      {.address = 1, .width = 1}, {.address = 49999, .width = 2}};
  EXPECT_EQ(NextFreeTransmissionAddress(used, {.min = 1, .max = 50000}), 2);
}

TEST(TransmissionAddressesOverlapTest, EqualAddressesOverlap) {
  EXPECT_TRUE(TransmissionAddressesOverlap({.address = 7}, {.address = 7}));
  EXPECT_FALSE(TransmissionAddressesOverlap({.address = 7}, {.address = 8}));
}

TEST(TransmissionAddressesOverlapTest, ARuleInsideAWideRuleOverlapsIt) {
  const TransmissionAddressSpan float_rule{.address = 40001, .width = 2};
  EXPECT_TRUE(TransmissionAddressesOverlap(float_rule, {.address = 40002}));
  EXPECT_TRUE(TransmissionAddressesOverlap({.address = 40002}, float_rule));
  EXPECT_FALSE(TransmissionAddressesOverlap(float_rule, {.address = 40003}));
  EXPECT_FALSE(TransmissionAddressesOverlap(float_rule, {.address = 40000}));
}

TEST(TransmissionAddressesOverlapTest, TwoWideRulesOverlapByTheirTails) {
  EXPECT_TRUE(TransmissionAddressesOverlap({.address = 40001, .width = 4},
                                           {.address = 40004, .width = 2}));
  EXPECT_FALSE(TransmissionAddressesOverlap({.address = 40001, .width = 4},
                                            {.address = 40005, .width = 2}));
}

TEST(TransmissionAddressesOverlapTest, TheTopOfTheInt32RangeDoesNotOverflow) {
  EXPECT_TRUE(TransmissionAddressesOverlap({.address = INT32_MAX, .width = 4},
                                           {.address = INT32_MAX}));
}

TEST(CoveredTransmissionAddressesTest, ListsEveryRegisterOfEachRule) {
  const std::vector<TransmissionAddressSpan> used{
      {.address = 40001, .width = 2}, {.address = 40010, .width = 1}};
  EXPECT_EQ(CoveredTransmissionAddresses(used),
            (std::set<int>{40001, 40002, 40010}));
}

// The widths are the Modbus tier's (`ParseChannelParams`): two registers for
// the 32-bit types, four for DOUBLE, one for everything else.
TEST(ModbusTransmissionRegisterCountTest, RegisterTablesTakeTheTypesWidth) {
  for (int address : {30001, 40001, 50000}) {
    EXPECT_EQ(ModbusTransmissionRegisterCount(address, ""), 1) << address;
    EXPECT_EQ(ModbusTransmissionRegisterCount(address, "UINT16"), 1);
    EXPECT_EQ(ModbusTransmissionRegisterCount(address, "INT16"), 1);
    EXPECT_EQ(ModbusTransmissionRegisterCount(address, "INT8"), 1);
    EXPECT_EQ(ModbusTransmissionRegisterCount(address, "BOOL"), 1);
    EXPECT_EQ(ModbusTransmissionRegisterCount(address, "FLOAT"), 2);
    EXPECT_EQ(ModbusTransmissionRegisterCount(address, "INT32"), 2);
    EXPECT_EQ(ModbusTransmissionRegisterCount(address, "UINT32"), 2);
    EXPECT_EQ(ModbusTransmissionRegisterCount(address, "DOUBLE"), 4);
  }
}

// The type matches without regard to case, and the options after ';' do not
// change the width.
TEST(ModbusTransmissionRegisterCountTest, ReadsTheTypeBeforeTheOptions) {
  EXPECT_EQ(ModbusTransmissionRegisterCount(40001, "float;swapwords"), 2);
  EXPECT_EQ(ModbusTransmissionRegisterCount(40001, "DOUBLE;reversewords"), 4);
  EXPECT_EQ(ModbusTransmissionRegisterCount(40001, ";swapbytes"), 1);
}

// Coils and Discrete Inputs carry one bit per rule whatever the format says
// (the tier rejects a wider type there).
TEST(ModbusTransmissionRegisterCountTest, BitTablesAreAlwaysOneWide) {
  EXPECT_EQ(ModbusTransmissionRegisterCount(1, "FLOAT"), 1);
  EXPECT_EQ(ModbusTransmissionRegisterCount(10001, "DOUBLE"), 1);
}

// A format the tier rejects sends nothing; it keeps its one address reserved.
TEST(ModbusTransmissionRegisterCountTest, ARejectedFormatIsOneWide) {
  EXPECT_EQ(ModbusTransmissionRegisterCount(40001, "REAL"), 1);
  EXPECT_EQ(ModbusTransmissionRegisterCount(40001, "FLOAT:40001"), 1);
  EXPECT_EQ(ModbusTransmissionRegisterCount(40001, "HOLDREG"), 1);
}

}  // namespace
