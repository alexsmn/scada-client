#pragma once

#include "base/awaitable.h"
#include "node_service/node_ref.h"

#include <cstdint>
#include <optional>
#include <set>
#include <span>
#include <string_view>
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

// The destination addresses one rule occupies: `width` consecutive addresses
// starting at `address`. Every rule takes one address, except a MODBUS rule
// whose Value format names a type wider than a register (see
// `ModbusTransmissionRegisterCount`).
struct TransmissionAddressSpan {
  int address = 0;
  int width = 1;

  // The last address the rule occupies. 64-bit so that a rule at the top of
  // the Int32 range cannot overflow.
  std::int64_t last() const { return std::int64_t{address} + width - 1; }

  bool operator==(const TransmissionAddressSpan&) const = default;
};

// True when `a` and `b` share at least one address. Two such rules write the
// same register (or IOA), so the receiver sees one value under both.
bool TransmissionAddressesOverlap(const TransmissionAddressSpan& a,
                                  const TransmissionAddressSpan& b);

// How many consecutive addresses a MODBUS retransmission rule at `address`
// with Value format `format` occupies, as the Modbus tier lays it out
// (`ParseTransmissionChannelParams` and `ParseChannelParams` in
// scada-tier-modbus/modules/modbus/modbus_channel_params.cpp, restated
// because the client cannot link a tier): in Input or Holding Registers,
// UINT32, INT32 and FLOAT take 2 registers and DOUBLE takes 4; every other
// type, an empty format, a bit table (Coils, Discrete Inputs) and a format the
// tier rejects (an unknown type, a location or bit part, or an option other
// than SwapBytes, SwapWords and ReverseWords) take 1. A rejected format sends
// nothing, but the address it names is still the one the engineer meant, so it
// keeps reserving it.
int ModbusTransmissionRegisterCount(int address, std::string_view format);

// The address a new rule on a table holding `used` should get. A new rule
// carries no Value format, so it takes one address.
//
// **The address after the highest one used**, not the lowest gap, counting
// every address a wide rule occupies: after a FLOAT rule at 40001 the next is
// 40003, never 40002, which is the FLOAT's second register. A destination's
// address map is agreed with the receiving dispatch centre, and a gap in it is
// as likely to be a reserved spare as an oversight; filling it silently would
// put a signal where the receiver expects something else. Max+1 appends after
// everything the receiver already knows, is predictable, and makes a run of
// ticks come out consecutive. Only when max+1 leaves `range` is the lowest
// free address in the range taken instead. An empty table starts at
// `range.min`. Null when every address in the range is taken.
std::optional<int> NextFreeTransmissionAddress(
    std::span<const TransmissionAddressSpan> used,
    TransmissionAddressRange range);

// Every address the rules in `used` occupy, each wide rule contributing all
// of its addresses. What a new one-address rule must not land on.
std::set<int> CoveredTransmissionAddresses(
    std::span<const TransmissionAddressSpan> used);

// The address `rule` (a TransmissionItemType instance) carries, or null while
// its value has not been fetched. Never a guessed 0: an unfetched rule must
// count neither as a duplicate nor as used.
std::optional<int> ReadTransmissionAddress(const NodeRef& rule);

// The addresses `rule` occupies: its address, and for a MODBUS rule the width
// its Value format gives (one while the format is not resident). Null while
// the address has not been fetched.
std::optional<TransmissionAddressSpan> ReadTransmissionAddressSpan(
    const NodeRef& rule);

// The known address spans of the transmission rules under `device`. Requires
// the device's children and their property instances fetched; a rule whose
// address is not resident yet is left out.
std::vector<TransmissionAddressSpan> ReadTransmissionAddresses(
    const NodeRef& device);

// The address range a rule on `device` may carry, given the addresses its
// rules already use. Per protocol, from the device's rule type:
//
//   IEC 60870-5   1 .. 2^(8 x octets) - 1, the octets being the link's
//                 information-object-address size (3 when not resident);
//                 address 0 is the standard's "irrelevant" IOA.
//   Modbus        the 10 000-address block of the highest used address
//                 (coils 1-10000, discrete inputs 10001-20000, input
//                 registers 30001-40000, holding registers 40001-50000), so a
//                 new rule stays in the same register table; the holding
//                 registers when the table is empty, since a coil carries
//                 only one bit. The encoding is the Modbus tier's
//                 (`EncodeModbusAddress`), restated here because the client
//                 cannot link a tier.
//   otherwise     1 .. INT32_MAX, the property's own type.
TransmissionAddressRange TransmissionAddressRangeFor(
    const NodeRef& device,
    std::span<const TransmissionAddressSpan> used);

// The address the next rule on `device` should get: the two functions above
// combined. Null when the range is full.
std::optional<int> NextFreeTransmissionAddressFor(
    const NodeRef& device,
    std::span<const TransmissionAddressSpan> used);

// Fetches what `ReadTransmissionAddresses` and `TransmissionAddressRangeFor`
// read for `device`: its children with their property instances, and its
// parent link's properties.
Awaitable<void> FetchTransmissionAddresses(NodeRef device);
