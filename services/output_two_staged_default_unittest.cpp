#include "services/output_two_staged_default.h"

#include "model/data_items_node_ids.h"
#include "model/devices_node_ids.h"
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

}  // namespace
