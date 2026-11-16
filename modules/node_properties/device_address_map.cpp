#include "node_properties/device_address_map.h"

#include "base/utf_convert.h"
#include "common/formula_util.h"
#include "model/data_items_node_ids.h"
#include "model/devices_node_ids.h"
#include "model/node_id_util.h"
#include "node_service/node_ref.h"
#include "node_service/node_service.h"
#include "node_service/node_util.h"
#include "scada/basic_types.h"
#include "scada/localized_text.h"
#include "scada/standard_node_ids.h"
#include "scada/variant.h"
#include "services/transmission_source.h"

#include <algorithm>
#include <array>
#include <cctype>
#include <charconv>
#include <string>
#include <utility>

namespace {

// Deep enough for any real group nesting; a bound so a reference cycle in a
// broken configuration cannot recurse forever.
constexpr int kMaxDataGroupDepth = 32;

// The channels a data item polls: the main one and its backup.
constexpr std::array kPolledChannelProperties = {
    scada::data_items::id::DataItemType_Input1,
    scada::data_items::id::DataItemType_Input2,
};

// True when `text` is a non-empty run of decimal digits that fits `value`.
bool ParseUnsigned(std::string_view text, unsigned& value) {
  if (text.empty())
    return false;
  const auto [ptr, ec] =
      std::from_chars(text.data(), text.data() + text.size(), value);
  return ec == std::errc{} && ptr == text.data() + text.size();
}

bool EqualsIgnoringCase(std::string_view a, std::string_view b) {
  return std::ranges::equal(a, b, [](char x, char y) {
    return std::toupper(static_cast<unsigned char>(x)) ==
           std::toupper(static_cast<unsigned char>(y));
  });
}

// An IEC 60870-5 channel's address part: `[BOOL:|INT16:|FLOAT:]<ioa>`. The
// grammar is the tier's `ParseIec60870ChannelParams`
// (scada-tier-iec104/modules/iec60870/module/iec60870_channel.h), restated
// because the Client cannot consume a tier; an unknown type prefix is refused
// there, so it is here.
std::optional<std::u16string> ParseIec60870Address(std::string_view nested) {
  if (const std::size_t colon = nested.find(':');
      colon != std::string_view::npos) {
    const std::string_view type = nested.substr(0, colon);
    if (type != "BOOL" && type != "INT16" && type != "FLOAT")
      return std::nullopt;
    nested = nested.substr(colon + 1);
  }
  unsigned ioa = 0;
  if (!ParseUnsigned(nested, ioa))
    return std::nullopt;
  return UtfConvert<char16_t>(std::to_string(ioa));
}

// A MODBUS channel's address part:
// `[LOCATION:][TYPE:]<address>[:<bit>[+<count>]][;option...]`, the tier's
// `ParseChannelParams` grammar
// (scada-tier-modbus/modules/modbus/modbus_channel_params.cpp, word lists in
// modbus_util.cpp), restated because the Client cannot consume a tier. The data
// type and the options say how the register is read, not which register it
// is, so they are dropped; a location word and a bit part do say which, so
// they stay. Like the tier, refuses an unknown word and a zero address.
std::optional<std::u16string> ParseModbusAddress(std::string_view nested) {
  static constexpr std::string_view kLocations[] = {"DISCINPUT", "COIL",
                                                    "INPUTREG", "HOLDREG"};
  static constexpr std::string_view kDataTypes[] = {
      "BOOL",  "UINT8", "UINT16", "UINT32", "INT8",
      "INT16", "INT32", "FLOAT",  "DOUBLE"};
  auto is_one_of = [](std::string_view token, const auto& words) {
    return std::ranges::any_of(words, [token](std::string_view word) {
      return EqualsIgnoringCase(word, token);
    });
  };

  std::string_view path = nested.substr(0, nested.find(';'));
  std::string address;
  bool has_number = false;
  for (;;) {
    const std::size_t colon = path.find(':');
    const std::string_view token = path.substr(0, colon);
    bool keep = true;
    if (!has_number) {
      unsigned number = 0;
      if (ParseUnsigned(token, number)) {
        if (number == 0)
          return std::nullopt;
        has_number = true;
      } else if (is_one_of(token, kDataTypes)) {
        keep = false;
      } else if (!is_one_of(token, kLocations)) {
        return std::nullopt;
      }
    }
    if (keep) {
      if (!address.empty())
        address += ':';
      address += token;
    }
    if (colon == std::string_view::npos)
      break;
    path = path.substr(colon + 1);
  }
  if (!has_number)
    return std::nullopt;
  return UtfConvert<char16_t>(address);
}

// Fetches `node`'s type chain, so IsInstanceOf and operator[](declaration_id)
// can resolve through it.
Awaitable<void> FetchTypeChain(NodeRef node) {
  for (NodeRef type = node.type_definition(); type;) {
    co_await type.Fetch(NodeFetchStatus::NodeAndChildren);
    type = type.supertype();
  }
}

// The polled row for `item`, when one of its channels is on `device_id`.
Awaitable<std::optional<AddressMapRow>> MakePolledRow(
    NodeRef item,
    PolledChannelProtocol protocol,
    scada::NodeId device_id) {
  for (const scada::NodeId& channel_id : kPolledChannelProperties) {
    NodeRef channel = item[channel_id];
    if (!channel)
      continue;
    // The property node's value needs its own fetch (children fetch loads the
    // node, not the value).
    co_await channel.Fetch(NodeFetchStatus::NodeOnly);
    const std::string formula = channel.value().get_or(std::string{});
    std::optional<std::u16string> address =
        GetPolledChannelAddress(protocol, device_id, formula);
    if (!address)
      continue;
    co_return AddressMapRow{
        .signal = ToString16(item.display_name()),
        .type = SignalTypeTag(item.type_definition().node_id()),
        .ioa = std::move(*address),
        .node_id = UtfConvert<char16_t>(NodeIdToScadaString(item.node_id())),
        .kind = AddressMapRow::Kind::kPolled};
  }
  co_return std::nullopt;
}

// Appends a row for every data item under `group`, recursively, that polls
// `device_id`. Data items are not linked to the device they poll — the link is
// the channel formula on the item — so the folder is walked.
Awaitable<void> AppendPolledRows(NodeRef group,
                                 PolledChannelProtocol protocol,
                                 scada::NodeId device_id,
                                 int depth,
                                 std::vector<AddressMapRow>& rows) {
  co_await group.Fetch(NodeFetchStatus::NodeAndChildren);
  for (NodeRef& child : group.targets(scada::id::Organizes)) {
    co_await child.Fetch(NodeFetchStatus::NodeAndChildren);
    co_await FetchTypeChain(child);

    if (IsInstanceOf(child, scada::data_items::id::DataItemType)) {
      if (std::optional<AddressMapRow> row =
              co_await MakePolledRow(child, protocol, device_id)) {
        rows.push_back(std::move(*row));
      }
    } else if (depth < kMaxDataGroupDepth &&
               IsInstanceOf(child, scada::data_items::id::DataGroupType)) {
      co_await AppendPolledRows(child, protocol, device_id, depth + 1, rows);
    }
  }
}

}  // namespace

std::u16string SignalTypeTag(const scada::NodeId& type_definition_id) {
  if (type_definition_id == scada::data_items::id::DiscreteItemType)
    return u"TS";
  if (type_definition_id == scada::data_items::id::AnalogItemType)
    return u"TI";
  return {};
}

std::optional<std::u16string> GetPolledChannelAddress(
    PolledChannelProtocol protocol,
    const scada::NodeId& device_id,
    std::string_view formula) {
  if (device_id.is_null() || formula.empty())
    return std::nullopt;
  const scada::NodeId channel_id = GetFormulaSingleNodeId(formula);
  if (channel_id.is_null())
    return std::nullopt;
  scada::NodeId root_id;
  std::string_view nested_name;
  if (!scada::GetRootNestedNodeId(channel_id, root_id, nested_name) ||
      root_id != device_id) {
    return std::nullopt;
  }
  switch (protocol) {
    case PolledChannelProtocol::kIec60870:
      return ParseIec60870Address(nested_name);
    case PolledChannelProtocol::kModbus:
      return ParseModbusAddress(nested_name);
  }
  return std::nullopt;
}

Awaitable<std::vector<AddressMapRow>> BuildDeviceAddressMap(
    AnyExecutor executor,
    NodeRef device) {
  std::vector<AddressMapRow> rows;

  co_await device.Fetch(NodeFetchStatus::NodeAndChildren);
  for (NodeRef& child : device.targets(scada::id::Organizes)) {
    co_await child.Fetch(NodeFetchStatus::NodeAndChildren);

    // Fetch the child's type chain first: operator[](declaration_id) maps the
    // SourceNode/Address aggregate declarations to instance properties
    // through the type, so the declarations must be resident.
    co_await FetchTypeChain(child);

    // A transmission item links a source data item to a protocol address via
    // the SourceNode NodeId property (transmission OPC UA alignment, phase 4);
    // a child without that property is some other node. The property node's
    // value needs its own fetch (children fetch loads the node, not the
    // value).
    // A 2.5.7 Server has no SourceNode property and links the item through
    // the 2.5 HasTransmissionSource reference instead, which the child's own
    // fetch already made resident (GetTransmissionSourceId).
    NodeRef source_prop =
        child[scada::devices::id::TransmissionItemType_SourceNode];
    if (source_prop)
      co_await source_prop.Fetch(NodeFetchStatus::NodeOnly);
    const scada::NodeId source_id = GetTransmissionSourceId(child);
    if (source_id.is_null())
      continue;
    NodeRef source = device.service()->GetNode(source_id);
    co_await source.Fetch(NodeFetchStatus::NodeOnly);

    // Fetch the source-address property node itself so its Value attribute is
    // resident (children fetch loads the property node but not its value).
    scada::Int32 ioa = 0;
    if (NodeRef address =
            child[scada::devices::id::TransmissionItemType_Address]) {
      co_await address.Fetch(NodeFetchStatus::NodeOnly);
      ioa = address.value().get_or<scada::Int32>(0);
    }

    AddressMapRow row;
    row.signal = ToString16(source.display_name());
    row.type = SignalTypeTag(source.type_definition().node_id());
    row.ioa = UtfConvert<char16_t>(std::to_string(ioa));
    row.node_id = UtfConvert<char16_t>(NodeIdToScadaString(source.node_id()));
    rows.push_back(std::move(row));
  }

  // The data items the device polls. Only the protocols whose channels are
  // addresses nested under the device: an IEC 61850 channel is a real node,
  // and its "address" is its path, which the node id already shows.
  co_await FetchTypeChain(device);
  std::optional<PolledChannelProtocol> protocol;
  if (IsInstanceOf(device, scada::devices::id::Iec60870DeviceType))
    protocol = PolledChannelProtocol::kIec60870;
  else if (IsInstanceOf(device, scada::devices::id::ModbusDeviceType))
    protocol = PolledChannelProtocol::kModbus;
  if (protocol) {
    co_await AppendPolledRows(
        device.service()->GetNode(scada::data_items::id::DataItems), *protocol,
        device.node_id(), /*depth=*/0, rows);
  }

  co_return rows;
}
