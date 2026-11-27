#include "parameter_form/address_map_row.h"

#include <gtest/gtest.h>

namespace {

using Kind = AddressMapRow::Kind;

AddressMapRow Row(std::u16string ioa, Kind kind = Kind::kTransmission) {
  return {.signal = u"s", .type = u"TI", .ioa = std::move(ioa), .kind = kind};
}

// UX audit 2026-11-25, item 4.1: two retransmitted signals at address 2001
// were shown side by side with no warning.
TEST(FindDuplicateAddressesTest, FlagsEveryRowSharingAnAddress) {
  const std::vector<AddressMapRow> rows{Row(u"1001"), Row(u"2001"),
                                        Row(u"2001"), Row(u"1002")};
  EXPECT_EQ(FindDuplicateAddresses(rows),
            (std::vector<bool>{false, true, true, false}));
}

// A polled signal and a retransmitted one at the same address go in opposite
// directions; that is not a conflict.
TEST(FindDuplicateAddressesTest, DirectionsDoNotConflict) {
  const std::vector<AddressMapRow> rows{Row(u"2001", Kind::kTransmission),
                                        Row(u"2001", Kind::kPolled)};
  EXPECT_EQ(FindDuplicateAddresses(rows), (std::vector<bool>{false, false}));
}

TEST(FindDuplicateAddressesTest, RowsWithoutAnAddressNeverConflict) {
  const std::vector<AddressMapRow> rows{Row(u""), Row(u"")};
  EXPECT_EQ(FindDuplicateAddresses(rows), (std::vector<bool>{false, false}));
}

}  // namespace
