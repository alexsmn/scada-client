#include "services/output_two_staged_default.h"

#include "common/formula_util.h"
#include "model/data_items_node_ids.h"
#include "model/devices_node_ids.h"
#include "model/node_id_util.h"
#include "node_service/test/fake_node_service.h"
#include "scada/standard_node_ids.h"

#include <gtest/gtest.h>

namespace {

// The device types the default is decided from, with the supertype link that
// `IsInstanceOf` walks, so a device of a protocol subtype resolves the same
// way it does against the Server's address space.
class OutputTwoStagedDefaultTest : public ::testing::Test {
 protected:
  OutputTwoStagedDefaultTest() {
    AddType(scada::devices::id::DeviceType, scada::NodeId{});
    AddType(scada::devices::id::ModbusDeviceType,
            scada::devices::id::DeviceType);
    AddType(scada::devices::id::Iec60870DeviceType,
            scada::devices::id::DeviceType);
    AddType(scada::devices::id::Iec61850DeviceType,
            scada::devices::id::DeviceType);
  }

  void AddType(const scada::NodeId& type_id, const scada::NodeId& supertype) {
    node_service_.Add(
        scada::NodeState{.node_id = type_id,
                         .node_class = scada::NodeClass::ObjectType,
                         .supertype_id = supertype});
  }

  NodeRef AddDevice(const scada::NodeId& type_id) {
    return node_service_.Add(
        scada::NodeState{.node_id = scada::NodeId{++next_device_, 2},
                         .node_class = scada::NodeClass::Object,
                         .type_definition_id = type_id});
  }

  FakeNodeService node_service_;
  scada::NumericId next_device_ = 0;
};

const scada::NodeId kAnalog = scada::data_items::id::AnalogItemType;
const scada::NodeId kDiscrete = scada::data_items::id::DiscreteItemType;

// MODBUS has no select-before-operate, so neither kind of item may default to
// two stages.
TEST_F(OutputTwoStagedDefaultTest, ModbusItemsAreOneStage) {
  NodeRef device = AddDevice(scada::devices::id::ModbusDeviceType);
  EXPECT_EQ(GetOutputTwoStagedDefault(device, kAnalog), false);
  EXPECT_EQ(GetOutputTwoStagedDefault(device, kDiscrete), false);
}

// An IEC 60870-5 set-point cannot be selected, while a single command can.
TEST_F(OutputTwoStagedDefaultTest, Iec60870OnlyTheAnalogItemIsOneStage) {
  NodeRef device = AddDevice(scada::devices::id::Iec60870DeviceType);
  EXPECT_EQ(GetOutputTwoStagedDefault(device, kAnalog), false);
  EXPECT_EQ(GetOutputTwoStagedDefault(device, kDiscrete), std::nullopt);
}

// The control model that decides it is on the IED, not in the configuration.
TEST_F(OutputTwoStagedDefaultTest, Iec61850KeepsTheDefault) {
  NodeRef device = AddDevice(scada::devices::id::Iec61850DeviceType);
  EXPECT_EQ(GetOutputTwoStagedDefault(device, kAnalog), std::nullopt);
  EXPECT_EQ(GetOutputTwoStagedDefault(device, kDiscrete), std::nullopt);
}

// A device of an unrecognised type, or no device at all, says nothing.
TEST_F(OutputTwoStagedDefaultTest, UnknownOrMissingDeviceKeepsTheDefault) {
  EXPECT_EQ(GetOutputTwoStagedDefault(AddDevice(scada::devices::id::DeviceType),
                                      kAnalog),
            std::nullopt);
  EXPECT_EQ(GetOutputTwoStagedDefault(NodeRef{}, kAnalog), std::nullopt);
  EXPECT_EQ(GetOutputTwoStagedDefault(
                node_service_.GetNode(scada::NodeId{999, 2}), kAnalog),
            std::nullopt);
}

// --- Which device a data item's command reaches -----------------------------

// The Output channel decides; Input1 stands in only while there is no Output.
TEST_F(OutputTwoStagedDefaultTest, DeviceComesFromOutputThenInput) {
  NodeRef modbus = AddDevice(scada::devices::id::ModbusDeviceType);
  NodeRef iec104 = AddDevice(scada::devices::id::Iec60870DeviceType);
  const std::string on_modbus =
      MakeNodeIdFormula(scada::MakeNestedNodeId(modbus.node_id(), "1"));
  const std::string on_iec104 =
      MakeNodeIdFormula(scada::MakeNestedNodeId(iec104.node_id(), "1/2"));

  EXPECT_EQ(GetDataItemDeviceId(node_service_, on_modbus, on_iec104),
            modbus.node_id());
  EXPECT_EQ(GetDataItemDeviceId(node_service_, "", on_iec104),
            iec104.node_id());
  EXPECT_TRUE(GetDataItemDeviceId(node_service_, "", "").is_null());
}

// An IEC 61850 data object is a real node under its device, not a nested id,
// so it resolves through its parents.
TEST_F(OutputTwoStagedDefaultTest, DeviceOfARealChannelNodeIsItsAncestor) {
  NodeRef device = AddDevice(scada::devices::id::Iec61850DeviceType);
  const scada::NodeId logical_node{500, 2};
  node_service_.Add(
      scada::NodeState{.node_id = logical_node,
                       .node_class = scada::NodeClass::Object,
                       .parent_id = device.node_id(),
                       .reference_type_id = scada::id::Organizes});
  const scada::NodeId data_object{501, 2};
  node_service_.Add(
      scada::NodeState{.node_id = data_object,
                       .node_class = scada::NodeClass::Variable,
                       .parent_id = logical_node,
                       .reference_type_id = scada::id::Organizes});

  EXPECT_EQ(
      GetDataItemDeviceId(node_service_, MakeNodeIdFormula(data_object), ""),
      device.node_id());
}

// --- When a device change rewrites the value --------------------------------

TEST(OutputTwoStagedOnRebindTest, FollowsTheDeviceOnlyFromTheOldDefault) {
  // Unbound (default Yes) → MODBUS: turns off.
  EXPECT_EQ(GetOutputTwoStagedOnRebind(true, std::nullopt, false), false);
  // MODBUS default (No) → a device that selects: back to Yes.
  EXPECT_EQ(GetOutputTwoStagedOnRebind(false, false, std::nullopt), true);
  // Yes on MODBUS was somebody's choice: kept.
  EXPECT_EQ(GetOutputTwoStagedOnRebind(true, false, std::nullopt),
            std::nullopt);
  // No on an unbound item was somebody's choice: kept.
  EXPECT_EQ(GetOutputTwoStagedOnRebind(false, std::nullopt, std::nullopt),
            std::nullopt);
  // Already what the new device wants: nothing to write.
  EXPECT_EQ(GetOutputTwoStagedOnRebind(false, false, false), std::nullopt);
}

}  // namespace
