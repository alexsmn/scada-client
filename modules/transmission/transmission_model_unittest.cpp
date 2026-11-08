#include "modules/transmission/transmission_model.h"

#include "address_space/test/test_scada_node_states.h"
#include "aui/models/table_column.h"
#include "base/test/test_executor.h"
#include "common/node_state.h"
#include "model/data_items_node_ids.h"
#include "model/devices_node_ids.h"
#include "node_service/static/static_node_service.h"
#include "services/task_manager_mock.h"
#include "services/transmission_source.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <memory>
#include <optional>
#include <vector>

namespace {

using testing::_;
using testing::NiceMock;

const scada::NodeId kLinkId{6001, 1};
const scada::NodeId kDeviceId{6002, 1};
const scada::NodeId kSourceId{6100, 1};
const scada::NodeId kTickedId{6101, 1};
const scada::NodeId kSecondTickedId{6102, 1};

// The insert's answer; the model posts and does not wait for it.
scada::CoStatusOr<scada::NodeId> AcceptInsert() {
  co_return scada::NodeId{};
}

std::optional<scada::Int32> AddressOf(const scada::NodeState& node_state) {
  for (const auto& [prop_decl_id, value] : node_state.properties) {
    if (prop_decl_id == scada::devices::id::TransmissionItemType_Address)
      return value.get_or(scada::Int32{0});
  }
  return std::nullopt;
}

// A destination device on a link, with its rules, in a real static node
// service: the model reads addresses through `NodeRef` exactly as it does
// against a server.
class TransmissionModelTest : public ::testing::Test {
 protected:
  void SetUp() override {
    node_service_.AddAll(GetScadaNodeStates());

    // The shared test address space carries the Modbus rule type only; the
    // IEC 60870 one and the link's IOA-width declaration are added here,
    // mirroring devices_iec60870.xml.
    node_service_.Add(scada::NodeState{
        .node_id = scada::devices::id::Iec60870TransmissionItemType,
        .node_class = scada::NodeClass::ObjectType,
        .parent_id = scada::devices::id::TransmissionItemType,
        .reference_type_id = scada::id::HasSubtype,
        .attributes = {.browse_name = "Iec60870TransmissionItemType"},
        .supertype_id = scada::devices::id::TransmissionItemType});
    node_service_.Add(scada::NodeState{
        .node_id = scada::devices::id::Iec60870LinkType_InfoAddressSize,
        .node_class = scada::NodeClass::Variable,
        .type_definition_id = scada::id::PropertyType,
        .parent_id = scada::devices::id::Iec60870LinkType,
        .reference_type_id = scada::id::HasProperty,
        .attributes = {.browse_name = "InfoAddressSize",
                       .data_type = scada::id::Int32}});

    node_service_.Add(scada::NodeState{
        .node_id = kSourceId,
        .node_class = scada::NodeClass::Variable,
        .type_definition_id = scada::data_items::id::AnalogItemType,
        .attributes = {.display_name = u"Ua"}});

    ON_CALL(task_manager_, PostInsertTask(_))
        .WillByDefault([this](const scada::NodeState& node_state) {
          inserted_.push_back(node_state);
          return AcceptInsert();
        });
  }

  void AddLink(
      const scada::NodeId& link_type_id,
      scada::NodeProperties properties = {},
      const scada::NodeId& device_type_id = scada::devices::id::DeviceType) {
    node_service_.Add(
        scada::NodeState{.node_id = kLinkId,
                         .node_class = scada::NodeClass::Object,
                         .type_definition_id = link_type_id,
                         .parent_id = scada::devices::id::Devices,
                         .reference_type_id = scada::id::Organizes,
                         .properties = std::move(properties)});
    node_service_.Add(
        scada::NodeState{.node_id = kDeviceId,
                         .node_class = scada::NodeClass::Object,
                         .type_definition_id = device_type_id,
                         .parent_id = kLinkId,
                         .reference_type_id = scada::id::Organizes});
  }

  void AddRule(int index,
               const scada::NodeId& rule_type_id,
               scada::Int32 address) {
    node_service_.Add(scada::NodeState{
        .node_id =
            scada::NodeId{static_cast<scada::NumericId>(6200 + index), 1},
        .node_class = scada::NodeClass::Object,
        .type_definition_id = rule_type_id,
        .parent_id = kDeviceId,
        .reference_type_id = scada::id::Organizes,
        .properties = {
            {scada::devices::id::TransmissionItemType_SourceNode, kSourceId},
            {scada::devices::id::TransmissionItemType_Address, address}}});
  }

  void Init() {
    model_->Init(node_service_.GetNode(kDeviceId));
    executor_.Poll();
  }

  std::u16string StatusText(int row) {
    scada::aui::GridCell cell{.row = row, .column = 2};
    model_->GetCell(cell);
    return cell.text;
  }

  TestExecutor executor_;
  StaticNodeService node_service_;
  NiceMock<MockTaskManager> task_manager_;
  std::vector<scada::NodeState> inserted_;
  std::shared_ptr<TransmissionModel> model_ =
      std::make_shared<TransmissionModel>(executor_,
                                          node_service_,
                                          task_manager_);
};

// A 2.5.7 Server has no SourceNode property: it links a rule to its source
// with the 2.5 HasTransmissionSource reference. The 2.6 Client used to read
// only the property, so every source on such a Server showed blank.
TEST_F(TransmissionModelTest, ARuleFromA257ServerShowsItsSource) {
  AddLink(scada::devices::id::ModbusLinkType);
  node_service_.Add(scada::NodeState{
      .node_id = scada::NodeId{6300, 1},
      .node_class = scada::NodeClass::Object,
      .type_definition_id = scada::devices::id::ModbusTransmissionItemType,
      .parent_id = kDeviceId,
      .reference_type_id = scada::id::Organizes,
      .properties = {{scada::devices::id::TransmissionItemType_Address,
                      scada::Int32{40001}}}}
                        .add_reference(scada::ReferenceDescription{
                            .reference_type_id = kLegacyHasTransmissionSource,
                            .forward = true,
                            .node_id = kSourceId}));
  Init();
  ASSERT_EQ(model_->GetRowCount(), 1);

  scada::aui::GridCell cell{.row = 0, .column = 0};
  model_->GetCell(cell);
  EXPECT_EQ(cell.text, u"Ua");
}

// The defect: a rule made by ticking an object carried no address, so it took
// the type's default of 1 and every ticked object shared it.
TEST_F(TransmissionModelTest, ATickedObjectGetsTheNextFreeAddress) {
  AddLink(scada::devices::id::ModbusLinkType);
  AddRule(0, scada::devices::id::ModbusTransmissionItemType, 40001);
  AddRule(1, scada::devices::id::ModbusTransmissionItemType, 40002);
  Init();
  ASSERT_EQ(model_->GetRowCount(), 2);

  model_->AddContainedItem(kTickedId, 0);

  ASSERT_EQ(inserted_.size(), 1u);
  EXPECT_EQ(AddressOf(inserted_[0]), 40003);
}

// The first rule on a Modbus receiver used to get coil 1, which carries one
// bit, so every analogue value arrived as 0 or 1 (backlog 987).
TEST_F(TransmissionModelTest, TheFirstModbusRuleIsTheFirstHoldingRegister) {
  AddLink(scada::devices::id::ModbusLinkType, {},
          scada::devices::id::ModbusDeviceType);
  Init();
  ASSERT_EQ(model_->GetRowCount(), 0);

  model_->AddContainedItem(kTickedId, 0);

  ASSERT_EQ(inserted_.size(), 1u);
  EXPECT_EQ(AddressOf(inserted_[0]), 40001);
}

// A receiver whose rules already use coils keeps giving coils.
TEST_F(TransmissionModelTest, AModbusTableOfCoilsStaysInCoils) {
  AddLink(scada::devices::id::ModbusLinkType);
  AddRule(0, scada::devices::id::ModbusTransmissionItemType, 7);
  Init();

  model_->AddContainedItem(kTickedId, 0);

  ASSERT_EQ(inserted_.size(), 1u);
  EXPECT_EQ(AddressOf(inserted_[0]), 8);
}

// Ticking several objects before the first insert comes back must not hand
// them all the same address computed from an unchanged table.
TEST_F(TransmissionModelTest, ObjectsTickedInARowGetDistinctAddresses) {
  AddLink(scada::devices::id::ModbusLinkType);
  AddRule(0, scada::devices::id::ModbusTransmissionItemType, 40001);
  Init();

  model_->AddContainedItem(kTickedId, 0);
  model_->AddContainedItem(kSecondTickedId, 0);

  ASSERT_EQ(inserted_.size(), 2u);
  EXPECT_EQ(AddressOf(inserted_[0]), 40002);
  EXPECT_EQ(AddressOf(inserted_[1]), 40003);
}

// A one-octet IEC 60870 link cannot carry an IOA above 255, so once the top
// is taken the lowest free address is used instead.
TEST_F(TransmissionModelTest, TheAddressStaysInsideTheLinksIoaWidth) {
  AddLink(scada::devices::id::Iec60870LinkType,
          {{scada::devices::id::Iec60870LinkType_InfoAddressSize,
            scada::Int32{1}}});
  AddRule(0, scada::devices::id::Iec60870TransmissionItemType, 1);
  AddRule(1, scada::devices::id::Iec60870TransmissionItemType, 255);
  Init();

  model_->AddContainedItem(kTickedId, 0);

  ASSERT_EQ(inserted_.size(), 1u);
  EXPECT_EQ(AddressOf(inserted_[0]), 2);
}

TEST_F(TransmissionModelTest, RulesSharingAnAddressAreFlagged) {
  AddLink(scada::devices::id::ModbusLinkType);
  AddRule(0, scada::devices::id::ModbusTransmissionItemType, 7);
  AddRule(1, scada::devices::id::ModbusTransmissionItemType, 8);
  AddRule(2, scada::devices::id::ModbusTransmissionItemType, 7);
  Init();
  ASSERT_EQ(model_->GetRowCount(), 3);

  std::vector<scada::Int32> addresses;
  for (int row = 0; row < model_->GetRowCount(); ++row) {
    addresses.push_back(
        model_->row(row)
            .transmission[scada::devices::id::TransmissionItemType_Address]
            .value()
            .get_or(scada::Int32{0}));
  }
  for (int row = 0; row < model_->GetRowCount(); ++row) {
    const bool duplicate = addresses[row] == 7;
    EXPECT_EQ(model_->IsDuplicateAddress(row), duplicate) << row;
    EXPECT_EQ(StatusText(row),
              duplicate ? u"Duplicate address" : std::u16string{})
        << row;
  }
}

// The Status column is read-only; only the address is edited in the grid.
TEST_F(TransmissionModelTest, OnlyTheAddressColumnIsEditable) {
  AddLink(scada::devices::id::ModbusLinkType);
  AddRule(0, scada::devices::id::ModbusTransmissionItemType, 7);
  Init();

  EXPECT_FALSE(model_->IsEditable(0, 0));
  EXPECT_TRUE(model_->IsEditable(0, 1));
  EXPECT_FALSE(model_->IsEditable(0, 2));
}

}  // namespace
