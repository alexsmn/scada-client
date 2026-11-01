#pragma once

#include "base/awaitable.h"
#include "node_service/node_ref.h"

#include <optional>
#include <set>
#include <span>
#include <vector>

// The destination addresses a device's transmission rules may carry, and how a
// new rule picks one. Shared by the Transmission table (a rule made by ticking
// an object) and the bulk-create wizard's caller (the IOA a run starts at), so
// both agree on what "the next free address" is.

// The inclusive range of addresses a rule on one destination device may carry.
struct TransmissionAddressRange {
  int min = 1;
  int max = 0x7fffffff;
};

// The address a new rule on a table holding `used` should get.
//
// **The highest used address plus one**, not the lowest gap. A destination's
// address map is agreed with the receiving dispatch centre, and a gap in it is
// as likely to be a reserved spare as an oversight; filling it silently would
// put a signal where the receiver expects something else. Max+1 appends after
// everything the receiver already knows, is predictable, and makes a run of
// ticks come out consecutive. Only when max+1 leaves `range` is the lowest
// free address in the range taken instead. An empty table starts at
// `range.min`. Null when every address in the range is taken.
std::optional<int> NextFreeTransmissionAddress(std::span<const int> used,
                                               TransmissionAddressRange range);

// The addresses that occur more than once in `addresses`. Two rules on one
// destination with one address send two objects under one address, and the
// receiver cannot tell them apart.
std::set<int> DuplicateTransmissionAddresses(std::span<const int> addresses);

// The address `rule` (a TransmissionItemType instance) carries, or null while
// its value has not been fetched. Never a guessed 0: an unfetched rule must
// count neither as a duplicate nor as used.
std::optional<int> ReadTransmissionAddress(const NodeRef& rule);

// The known addresses of the transmission rules under `device`. Requires the
// device's children and their property instances fetched; a rule whose
// address is not resident yet is left out.
std::vector<int> ReadTransmissionAddresses(const NodeRef& device);

// The address range a rule on `device` may carry, given the addresses its
// rules already use. Per protocol, from the device's rule type:
//
//   IEC 60870-5   1 .. 2^(8 x octets) - 1, the octets being the link's
//                 information-object-address size (3 when not resident);
//                 address 0 is the standard's "irrelevant" IOA.
//   Modbus        the 10 000-address block of the highest used address
//                 (coils 1-10000, discrete inputs 10001-20000, input
//                 registers 30001-40000, holding registers 40001-50000), so a
//                 new rule stays in the same register table; block 1 when the
//                 table is empty. The encoding is the Modbus tier's
//                 (`EncodeModbusAddress`), restated here because the client
//                 cannot link a tier.
//   otherwise     1 .. INT32_MAX, the property's own type.
TransmissionAddressRange TransmissionAddressRangeFor(const NodeRef& device,
                                                     std::span<const int> used);

// The address the next rule on `device` should get: the two functions above
// combined. Null when the range is full.
std::optional<int> NextFreeTransmissionAddressFor(const NodeRef& device,
                                                  std::span<const int> used);

// Fetches what `ReadTransmissionAddresses` and `TransmissionAddressRangeFor`
// read for `device`: its children with their property instances, and its
// parent link's properties.
Awaitable<void> FetchTransmissionAddresses(NodeRef device);
