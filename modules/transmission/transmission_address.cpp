#include "modules/transmission/transmission_address.h"

#include "model/devices_node_ids.h"
#include "model/scada_node_ids.h"
#include "node_service/node_awaitable.h"
#include "node_service/node_util.h"

#include <algorithm>
#include <cctype>
#include <climits>
#include <cstdint>
#include <string>

namespace {

// The Modbus tier's address encoding: location code x 10 000 + register + 1,
// so each register table owns one 10 000-wide block
// (scada-tier-modbus/modules/modbus/modbus_util.cpp, kLocationAddressRange).
constexpr int kModbusBlockSize = 10000;

// The holding registers (40001-50000), where an empty Modbus table starts: a
// master reads an analogue value there with function 3, and a discrete one as
// 0 or 1. Coils (block 0) carry one bit, so starting there truncated every
// analogue value to 0 or 1.
constexpr int kModbusHoldingRegistersBlock = 4;

// The input registers (30001-40000), the other table whose rules may carry a
// type wider than one register.
constexpr int kModbusInputRegistersBlock = 3;

// The IOA width when the link's own setting is not resident: the nodeset's
// default for Iec60870LinkType_InfoAddressSize.
constexpr int kDefaultIecInfoAddressOctets = 3;

bool IsRuleType(const NodeRef& device, const scada::NodeId& rule_type_id) {
  for (const NodeRef::Reference& reference :
       device.references(scada::id::Organizes)) {
    if (reference.target.type_definition().node_id() == rule_type_id)
      return true;
  }
  return false;
}

// The rule type the device's existing rules carry, or else the one its type
// chain declares. Read from the rules first because they are what the walk
// above fetched; the placeholder lookup needs the device type chain resident.
bool DeviceUsesRuleType(const NodeRef& device,
                        const scada::NodeId& rule_type_id) {
  if (IsRuleType(device, rule_type_id))
    return true;
  for (auto type = device.type_definition(); type; type = type.supertype()) {
    for (const scada::NodeId& reference_type_id :
         {scada::devices::id::HasTransmissionItem,
          scada::NodeId{scada::id::Organizes}}) {
      for (const NodeRef& placeholder : type.targets(reference_type_id)) {
        if (placeholder.type_definition().node_id() == rule_type_id)
          return true;
      }
    }
  }
  return false;
}

}  // namespace

bool TransmissionAddressesOverlap(const TransmissionAddressSpan& a,
                                  const TransmissionAddressSpan& b) {
  return a.address <= b.last() && b.address <= a.last();
}

int ModbusTransmissionRegisterCount(int address, std::string_view format) {
  // Only the register tables carry a wider type; in Coils and Discrete Inputs
  // the tier allows BOOL alone.
  if (address < 1)
    return 1;
  const int block = (address - 1) / kModbusBlockSize;
  if (block != kModbusInputRegistersBlock &&
      block != kModbusHoldingRegistersBlock)
    return 1;

  // The type is the format's first ';'-separated token, matched without
  // regard to case (`ParseDataType`). A location or bit part makes the tier
  // reject the whole format.
  const std::string_view type = format.substr(0, format.find(';'));
  if (type.find(':') != type.npos || type.find('+') != type.npos)
    return 1;
  auto is = [type](std::string_view name) {
    return std::ranges::equal(type, name, [](char a, char b) {
      return std::toupper(static_cast<unsigned char>(a)) == b;
    });
  };
  if (is("UINT32") || is("INT32") || is("FLOAT"))
    return 2;
  if (is("DOUBLE"))
    return 4;
  return 1;
}

std::set<int> CoveredTransmissionAddresses(
    std::span<const TransmissionAddressSpan> used) {
  std::set<int> covered;
  for (const TransmissionAddressSpan& span : used) {
    for (std::int64_t address = span.address;
         address <= span.last() && address <= INT32_MAX; ++address) {
      covered.insert(static_cast<int>(address));
    }
  }
  return covered;
}

std::optional<int> NextFreeTransmissionAddress(
    std::span<const TransmissionAddressSpan> used,
    TransmissionAddressRange range) {
  if (range.max < range.min)
    return std::nullopt;

  const std::set<int> taken = CoveredTransmissionAddresses(used);
  // Only the addresses inside the range matter: a stray out-of-range address
  // must neither push max+1 past the range nor count as occupying it.
  auto in_range_max = taken.upper_bound(range.max);
  if (in_range_max == taken.begin() || *std::prev(in_range_max) < range.min)
    return range.min;

  const int highest = *std::prev(in_range_max);
  if (highest < range.max)
    return highest + 1;

  // The top of the range is taken: fall back to the lowest gap.
  for (std::int64_t address = range.min; address <= range.max; ++address) {
    if (!taken.contains(static_cast<int>(address)))
      return static_cast<int>(address);
  }
  return std::nullopt;
}

std::optional<int> ReadTransmissionAddress(const NodeRef& rule) {
  const scada::Variant value =
      rule[scada::devices::id::TransmissionItemType_Address].value();
  scada::Int32 address = 0;
  if (value.is_null() || !value.get(address))
    return std::nullopt;
  return address;
}

std::optional<TransmissionAddressSpan> ReadTransmissionAddressSpan(
    const NodeRef& rule) {
  const std::optional<int> address = ReadTransmissionAddress(rule);
  if (!address)
    return std::nullopt;
  int width = 1;
  // Only a MODBUS rule has a Value format; an IEC 60870-5 rule is one IOA.
  if (IsInstanceOf(rule, scada::devices::id::ModbusTransmissionItemType)) {
    const std::string format =
        rule[scada::devices::id::ModbusTransmissionItemType_Format]
            .value()
            .get_or(std::string{});
    width = ModbusTransmissionRegisterCount(*address, format);
  }
  return TransmissionAddressSpan{.address = *address, .width = width};
}

std::vector<TransmissionAddressSpan> ReadTransmissionAddresses(
    const NodeRef& device) {
  std::vector<TransmissionAddressSpan> addresses;
  for (const NodeRef::Reference& reference :
       device.references(scada::id::Organizes)) {
    if (!IsInstanceOf(reference.target,
                      scada::devices::id::TransmissionItemType))
      continue;
    if (std::optional<TransmissionAddressSpan> span =
            ReadTransmissionAddressSpan(reference.target))
      addresses.push_back(*span);
  }
  return addresses;
}

TransmissionAddressRange TransmissionAddressRangeFor(
    const NodeRef& device,
    std::span<const TransmissionAddressSpan> used) {
  if (DeviceUsesRuleType(device,
                         scada::devices::id::Iec60870TransmissionItemType)) {
    int octets =
        device.parent()[scada::devices::id::Iec60870LinkType_InfoAddressSize]
            .value()
            .get_or(kDefaultIecInfoAddressOctets);
    if (octets < 1 || octets > 3)
      octets = kDefaultIecInfoAddressOctets;
    return {.min = 1, .max = (1 << (8 * octets)) - 1};
  }

  if (DeviceUsesRuleType(device,
                         scada::devices::id::ModbusTransmissionItemType)) {
    int block = kModbusHoldingRegistersBlock;
    if (!used.empty()) {
      const int highest =
          std::ranges::max_element(used, {}, &TransmissionAddressSpan::address)
              ->address;
      if (highest >= 1)
        block = (highest - 1) / kModbusBlockSize;
    }
    return {.min = block * kModbusBlockSize + 1,
            .max = (block + 1) * kModbusBlockSize};
  }

  return {};
}

std::optional<int> NextFreeTransmissionAddressFor(
    const NodeRef& device,
    std::span<const TransmissionAddressSpan> used) {
  return NextFreeTransmissionAddress(used,
                                     TransmissionAddressRangeFor(device, used));
}

Awaitable<void> FetchTransmissionAddresses(NodeRef device) {
  (void)co_await FetchChildrenStatus(device);
  (void)co_await FetchTypeChainStatus(device.type_definition());
  // The IOA width is a property of the link the device sits on.
  (void)co_await FetchChildrenStatus(device.parent());
  for (const NodeRef::Reference& reference :
       device.references(scada::id::Organizes)) {
    NodeRef rule = reference.target;
    (void)co_await FetchChildrenStatus(rule);
    (void)co_await FetchTypeChainStatus(rule.type_definition());
  }
}
