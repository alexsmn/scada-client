#include "node_properties/device_address_map.h"

#include "base/utf_convert.h"
#include "common/formula_util.h"
#include "model/data_items_node_ids.h"
#include "model/devices_node_ids.h"
#include "model/node_id_util.h"
#include "node_service/test/fake_node_service.h"
#include "scada/localized_text.h"
#include "scada/node_id.h"
#include "scada/standard_node_ids.h"

#include <boost/asio/io_context.hpp>
#include <gtest/gtest.h>

#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace {

TEST(SignalTypeTagTest, DiscreteItemIsTS) {
  EXPECT_EQ(SignalTypeTag(scada::data_items::id::DiscreteItemType), u"TS");
}

TEST(SignalTypeTagTest, AnalogItemIsTI) {
  EXPECT_EQ(SignalTypeTag(scada::data_items::id::AnalogItemType), u"TI");
}

TEST(SignalTypeTagTest, OtherTypeHasNoTag) {
  EXPECT_TRUE(SignalTypeTag(scada::NodeId{}).empty());
  EXPECT_TRUE(SignalTypeTag(scada::data_items::id::DataItemType).empty());
}

constexpr scada::NodeId kDevice{5, 2};
constexpr scada::NodeId kOtherDevice{6, 2};

// The formula a data item's channel holds for `address` on `device`.
std::string Channel(const scada::NodeId& device, std::string_view address) {
  return MakeNodeIdFormula(scada::MakeNestedNodeId(device, address));
}

std::optional<std::u16string> Iec(std::string_view formula) {
  return GetPolledChannelAddress(PolledChannelProtocol::kIec60870, kDevice,
                                 formula);
}

std::optional<std::u16string> Modbus(std::string_view formula) {
  return GetPolledChannelAddress(PolledChannelProtocol::kModbus, kDevice,
                                 formula);
}

TEST(GetPolledChannelAddressTest, Iec60870ReadsTheIoa) {
  EXPECT_EQ(Iec(Channel(kDevice, "105")), u"105");
  // A type override says how the value is sent, not where.
  EXPECT_EQ(Iec(Channel(kDevice, "FLOAT:4001")), u"4001");
  EXPECT_EQ(Iec(Channel(kDevice, "BOOL:7")), u"7");
}

TEST(GetPolledChannelAddressTest, Iec60870RefusesWhatTheTierRefuses) {
  EXPECT_EQ(Iec(Channel(kDevice, "DOUBLE:5")), std::nullopt);
  EXPECT_EQ(Iec(Channel(kDevice, "abc")), std::nullopt);
  EXPECT_EQ(Iec(Channel(kDevice, "FLOAT:")), std::nullopt);
}

TEST(GetPolledChannelAddressTest, ModbusKeepsTheRegisterDropsTheReading) {
  EXPECT_EQ(Modbus(Channel(kDevice, "40001")), u"40001");
  EXPECT_EQ(Modbus(Channel(kDevice, "FLOAT:40001;swapwords")), u"40001");
  EXPECT_EQ(Modbus(Channel(kDevice, "float:30002")), u"30002");
  // A location word and a bit part name the register, so they stay.
  EXPECT_EQ(Modbus(Channel(kDevice, "HOLDREG:1")), u"HOLDREG:1");
  EXPECT_EQ(Modbus(Channel(kDevice, "HOLDREG:UINT16:1:3+2")), u"HOLDREG:1:3+2");
  EXPECT_EQ(Modbus(Channel(kDevice, "40001:4")), u"40001:4");
}

TEST(GetPolledChannelAddressTest, ModbusRefusesWhatTheTierRefuses) {
  EXPECT_EQ(Modbus(Channel(kDevice, "0")), std::nullopt);
  EXPECT_EQ(Modbus(Channel(kDevice, "FLOAT")), std::nullopt);
  EXPECT_EQ(Modbus(Channel(kDevice, "REGISTER:5")), std::nullopt);
}

TEST(GetPolledChannelAddressTest, OnlyASingleChannelOfThisDevice) {
  EXPECT_EQ(Iec(Channel(kOtherDevice, "105")), std::nullopt);
  EXPECT_EQ(Iec(MakeNodeIdFormula(kDevice)), std::nullopt);
  EXPECT_EQ(Iec(""), std::nullopt);
  EXPECT_EQ(Iec(Channel(kDevice, "1") + " + " + Channel(kDevice, "2")),
            std::nullopt);
}

// Backlog 1075: the map was built from transmission items alone, so a device
// that only polls had no address-map tab and Device watch reported every IOA
// it decoded as unmapped.
class BuildDeviceAddressMapTest : public ::testing::Test {
 protected:
  static constexpr scada::NodeId kGroup{100, 2};
  static constexpr scada::NodeId kNestedGroup{101, 2};
  static constexpr scada::NodeId kTransmissionItemType{900, 12};

  BuildDeviceAddressMapTest() {
    AddType(scada::data_items::id::DataItemType, {},
            scada::NodeClass::VariableType);
    AddType(scada::data_items::id::DiscreteItemType,
            scada::data_items::id::DataItemType,
            scada::NodeClass::VariableType);
    AddType(scada::data_items::id::AnalogItemType,
            scada::data_items::id::DataItemType,
            scada::NodeClass::VariableType);
    AddType(scada::data_items::id::DataGroupType, {},
            scada::NodeClass::ObjectType);
    AddType(scada::devices::id::DeviceType, {}, scada::NodeClass::ObjectType);
    AddType(scada::devices::id::Iec60870DeviceType,
            scada::devices::id::DeviceType, scada::NodeClass::ObjectType);
    AddType(scada::devices::id::ModbusDeviceType,
            scada::devices::id::DeviceType, scada::NodeClass::ObjectType);
    AddType(scada::devices::id::Iec61850DeviceType,
            scada::devices::id::DeviceType, scada::NodeClass::ObjectType);
    AddType(kTransmissionItemType, {}, scada::NodeClass::ObjectType);

    nodes_.Add(scada::NodeState{.node_id = scada::data_items::id::DataItems,
                                .node_class = scada::NodeClass::Object});
    AddGroup(kGroup, scada::data_items::id::DataItems);
    AddGroup(kNestedGroup, kGroup);
  }

  void AddType(const scada::NodeId& type_id,
               const scada::NodeId& supertype,
               scada::NodeClass node_class) {
    nodes_.Add(scada::NodeState{.node_id = type_id,
                                .node_class = node_class,
                                .supertype_id = supertype});
  }

  void AddGroup(const scada::NodeId& id, const scada::NodeId& parent) {
    nodes_.Add(scada::NodeState{
        .node_id = id,
        .node_class = scada::NodeClass::Object,
        .type_definition_id = scada::data_items::id::DataGroupType,
        .parent_id = parent,
        .reference_type_id = scada::id::Organizes});
  }

  NodeRef AddDevice(const scada::NodeId& type_id) {
    return nodes_.Add(scada::NodeState{.node_id = kDevice,
                                       .node_class = scada::NodeClass::Object,
                                       .type_definition_id = type_id});
  }

  void AddItem(scada::NumericId id,
               std::u16string_view name,
               const scada::NodeId& type_id,
               const scada::NodeId& parent,
               std::string input1,
               std::string input2 = {}) {
    scada::NodeState state{
        .node_id = scada::NodeId{id, 2},
        .node_class = scada::NodeClass::Variable,
        .type_definition_id = type_id,
        .parent_id = parent,
        .reference_type_id = scada::id::Organizes,
        .attributes = {.display_name =
                           scada::ToLocalizedText(std::u16string{name})}};
    state.properties.emplace_back(scada::data_items::id::DataItemType_Input1,
                                  scada::Variant{std::move(input1)});
    if (!input2.empty()) {
      state.properties.emplace_back(scada::data_items::id::DataItemType_Input2,
                                    scada::Variant{std::move(input2)});
    }
    nodes_.Add(std::move(state));
  }

  std::vector<AddressMapRow> Build(NodeRef device) {
    std::vector<AddressMapRow> rows;
    RunAwaitable(io_, [&]() -> Awaitable<void> {
      rows = co_await BuildDeviceAddressMap(io_.get_executor(), device);
    });
    return rows;
  }

  boost::asio::io_context io_;
  FakeNodeService nodes_;
};

TEST_F(BuildDeviceAddressMapTest, Iec60870DeviceMapsTheItemsItPolls) {
  NodeRef device = AddDevice(scada::devices::id::Iec60870DeviceType);
  AddItem(1, u"Q1 state", scada::data_items::id::DiscreteItemType, kGroup,
          Channel(kDevice, "1001"));
  // Found in a nested group too, and with the type override dropped.
  AddItem(2, u"U L1-L2", scada::data_items::id::AnalogItemType, kNestedGroup,
          Channel(kDevice, "FLOAT:4001"));
  // Polled by another device: not this device's map.
  AddItem(3, u"Elsewhere", scada::data_items::id::DiscreteItemType, kGroup,
          Channel(kOtherDevice, "1001"));

  const std::vector<AddressMapRow> rows = Build(device);

  // The walk's order is the folder's, which is not the point here.
  ASSERT_EQ(rows.size(), 2u);
  const auto find =
      [&rows](std::u16string_view signal) -> const AddressMapRow& {
    return rows[0].signal == signal ? rows[0] : rows[1];
  };
  const AddressMapRow& q1 = find(u"Q1 state");
  EXPECT_EQ(q1.signal, u"Q1 state");
  EXPECT_EQ(q1.type, u"TS");
  EXPECT_EQ(q1.ioa, u"1001");
  EXPECT_EQ(q1.kind, AddressMapRow::Kind::kPolled);
  const AddressMapRow& u12 = find(u"U L1-L2");
  EXPECT_EQ(u12.signal, u"U L1-L2");
  EXPECT_EQ(u12.type, u"TI");
  EXPECT_EQ(u12.ioa, u"4001");
  EXPECT_EQ(u12.kind, AddressMapRow::Kind::kPolled);
  EXPECT_EQ(u12.node_id,
            UtfConvert<char16_t>(NodeIdToScadaString(scada::NodeId{2, 2})));
}

TEST_F(BuildDeviceAddressMapTest, BackupChannelOnTheDeviceIsMapped) {
  NodeRef device = AddDevice(scada::devices::id::Iec60870DeviceType);
  AddItem(1, u"Backup only", scada::data_items::id::DiscreteItemType, kGroup,
          Channel(kOtherDevice, "5"), Channel(kDevice, "17"));

  const std::vector<AddressMapRow> rows = Build(device);

  ASSERT_EQ(rows.size(), 1u);
  EXPECT_EQ(rows[0].ioa, u"17");
}

TEST_F(BuildDeviceAddressMapTest, ModbusDeviceMapsRegisters) {
  NodeRef device = AddDevice(scada::devices::id::ModbusDeviceType);
  AddItem(1, u"Power", scada::data_items::id::AnalogItemType, kGroup,
          Channel(kDevice, "FLOAT:40001;swapwords"));

  const std::vector<AddressMapRow> rows = Build(device);

  ASSERT_EQ(rows.size(), 1u);
  EXPECT_EQ(rows[0].ioa, u"40001");
  EXPECT_EQ(rows[0].kind, AddressMapRow::Kind::kPolled);
}

// An IEC 61850 channel is a real node, not an address under the device, so
// there is nothing to parse and the folder is not walked for it.
TEST_F(BuildDeviceAddressMapTest, Iec61850DeviceHasNoPolledRows) {
  NodeRef device = AddDevice(scada::devices::id::Iec61850DeviceType);
  AddItem(1, u"Q1 state", scada::data_items::id::DiscreteItemType, kGroup,
          Channel(kDevice, "1001"));

  EXPECT_TRUE(Build(device).empty());
  EXPECT_TRUE(nodes_.fetch_requests(kGroup).empty());
}

TEST_F(BuildDeviceAddressMapTest, TransmissionRowsComeFirstAndAreMarked) {
  NodeRef device = AddDevice(scada::devices::id::Iec60870DeviceType);
  AddItem(1, u"Polled", scada::data_items::id::DiscreteItemType, kGroup,
          Channel(kDevice, "1001"));
  AddItem(2, u"Served", scada::data_items::id::AnalogItemType, kGroup,
          Channel(kOtherDevice, "1"));
  nodes_.Add(scada::NodeState{
      .node_id = scada::NodeId{700, 2},
      .node_class = scada::NodeClass::Object,
      .type_definition_id = kTransmissionItemType,
      .parent_id = kDevice,
      .reference_type_id = scada::id::Organizes,
      .properties = {{scada::devices::id::TransmissionItemType_SourceNode,
                      scada::NodeId{2, 2}},
                     {scada::devices::id::TransmissionItemType_Address,
                      static_cast<scada::Int32>(2001)}}});

  const std::vector<AddressMapRow> rows = Build(device);

  ASSERT_EQ(rows.size(), 2u);
  EXPECT_EQ(rows[0].signal, u"Served");
  EXPECT_EQ(rows[0].ioa, u"2001");
  EXPECT_EQ(rows[0].kind, AddressMapRow::Kind::kTransmission);
  EXPECT_EQ(rows[1].signal, u"Polled");
  EXPECT_EQ(rows[1].kind, AddressMapRow::Kind::kPolled);
}

}  // namespace
