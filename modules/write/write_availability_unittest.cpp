#include "modules/write/write_availability.h"

#include "address_space/address_space_impl.h"
#include "address_space/generic_node_factory.h"
#include "address_space/standard_address_space.h"
#include "address_space/test/scada_test_address_space.h"
#include "base/check.h"
#include "common/node_state.h"
#include "model/data_items_node_ids.h"
#include "model/namespaces.h"
#include "node_service/node_ref.h"
#include "node_service/test/create_test_node_service.h"

#include <gtest/gtest.h>

namespace {

class WriteAvailabilityTest : public testing::Test {
 protected:
  void SetUp() override {
    scada_test::AddScadaDataItemsTestTypes(address_space_);
  }

  ~WriteAvailabilityTest() override { address_space_.Clear(); }

  // Creates a node of `node_class` typed `type_definition_id`, optionally
  // carrying an output channel.
  NodeRef CreateNode(unsigned id,
                     scada::NodeClass node_class,
                     const scada::NodeId& type_definition_id,
                     std::optional<std::u16string> output = std::nullopt,
                     std::optional<bool> two_staged = std::nullopt) {
    GenericNodeFactory node_factory{address_space_};
    scada::NodeState state{scada::NodeId{id, scada::NamespaceIndexes::SCADA},
                           node_class,
                           type_definition_id,
                           scada::data_items::id::DataItems,
                           scada::id::Organizes,
                           scada::NodeAttributes{.display_name = u"Signal"}};
    if (output) {
      state.properties.emplace_back(scada::data_items::id::DataItemType_Output,
                                    *output);
    }
    if (two_staged) {
      state.properties.emplace_back(
          scada::data_items::id::DataItemType_OutputTwoStaged, *two_staged);
    }
    const auto [status, node] = node_factory.CreateNode(state);
    scada::base::Check(status);
    return GetNode(node->id());
  }

  NodeRef GetNode(const scada::NodeId& node_id) {
    if (!node_service_)
      node_service_ = node_service::test::CreateTestNodeService(address_space_);
    return node_service_->GetNode(node_id);
  }

  AddressSpaceImpl address_space_;
  // The SCADA data-item types derive from the standard OPC UA types.
  StandardAddressSpace standard_address_space_{address_space_};
  std::shared_ptr<NodeService> node_service_;
};

// A folder or object carries no value to command.
TEST_F(WriteAvailabilityTest, NonVariableIsNotCommandable) {
  const NodeRef folder =
      CreateNode(1, scada::NodeClass::Object, scada::id::FolderType);
  EXPECT_EQ(GetWriteBlock(folder), WriteBlock::kNotCommandable);
}

// A null node — an expression row with no backing node — likewise.
TEST_F(WriteAvailabilityTest, NullNodeIsNotCommandable) {
  EXPECT_EQ(GetWriteBlock(NodeRef{}), WriteBlock::kNotCommandable);
}

// A data item drives its output channel; without one there is nothing to
// write to, so the command stays visible but disabled.
TEST_F(WriteAvailabilityTest, DataItemWithoutOutputHasNoChannel) {
  const NodeRef signal = CreateNode(2, scada::NodeClass::Variable,
                                    scada::data_items::id::AnalogItemType);
  EXPECT_EQ(GetWriteBlock(signal), WriteBlock::kNoOutputChannel);
}

TEST_F(WriteAvailabilityTest, DataItemWithOutputAcceptsControl) {
  const NodeRef signal =
      CreateNode(3, scada::NodeClass::Variable,
                 scada::data_items::id::AnalogItemType, u"TC.1");
  EXPECT_EQ(GetWriteBlock(signal), WriteBlock::kNone);
}

// A plain variable that is not a data item has no output-channel concept.
TEST_F(WriteAvailabilityTest, PlainVariableAcceptsControl) {
  const NodeRef variable =
      CreateNode(4, scada::NodeClass::Variable, scada::id::BaseVariableType);
  EXPECT_EQ(GetWriteBlock(variable), WriteBlock::kNone);
}

// Every blocking reason has operator-facing text; the unblocked case has none.
TEST_F(WriteAvailabilityTest, EveryBlockingReasonHasText) {
  EXPECT_FALSE(WriteBlockText(WriteBlock::kNotCommandable).empty());
  EXPECT_FALSE(WriteBlockText(WriteBlock::kNoOutputChannel).empty());
  EXPECT_TRUE(WriteBlockText(WriteBlock::kNone).empty());
}

// The Inspector's hint and the Control dialog read one rule (backlog 1074): a
// data item's "Two-stage control" decides, absent meaning the nodeset's Yes.
TEST_F(WriteAvailabilityTest, TwoStagedControlFollowsTheProperty) {
  const NodeRef one_stage =
      CreateNode(5, scada::NodeClass::Variable,
                 scada::data_items::id::AnalogItemType, u"TC.1", false);
  const NodeRef two_stage =
      CreateNode(6, scada::NodeClass::Variable,
                 scada::data_items::id::DiscreteItemType, u"TC.2", true);
  const NodeRef by_default =
      CreateNode(7, scada::NodeClass::Variable,
                 scada::data_items::id::DiscreteItemType, u"TC.3");

  EXPECT_FALSE(IsTwoStagedControl(one_stage, /*scada_server=*/true));
  EXPECT_TRUE(IsTwoStagedControl(two_stage, /*scada_server=*/true));
  EXPECT_TRUE(IsTwoStagedControl(by_default, /*scada_server=*/true));
}

// A foreign OPC UA server has no Control object to select, so even a Yes is
// written in one stage there.
TEST_F(WriteAvailabilityTest, ForeignServerControlIsOneStage) {
  const NodeRef two_stage =
      CreateNode(8, scada::NodeClass::Variable,
                 scada::data_items::id::DiscreteItemType, u"TC.2", true);
  EXPECT_FALSE(IsTwoStagedControl(two_stage, /*scada_server=*/false));
}

// Each of the four flows reads differently, and only the two-stage ones
// mention stages; only the confirmed ones promise a confirmation.
TEST_F(WriteAvailabilityTest, ControlFlowTextNamesStagesAndConfirmation) {
  const std::string_view two_confirmed = ControlFlowText(true, true);
  const std::string_view two_unconfirmed = ControlFlowText(true, false);
  const std::string_view one_confirmed = ControlFlowText(false, true);
  const std::string_view one_unconfirmed = ControlFlowText(false, false);

  EXPECT_NE(two_confirmed.find("two stages"), std::string_view::npos);
  EXPECT_NE(two_unconfirmed.find("two stages"), std::string_view::npos);
  EXPECT_EQ(one_confirmed.find("two stages"), std::string_view::npos);
  EXPECT_EQ(one_unconfirmed.find("two stages"), std::string_view::npos);

  EXPECT_NE(two_confirmed.find("you confirm"), std::string_view::npos);
  EXPECT_NE(one_confirmed.find("You confirm"), std::string_view::npos);
  EXPECT_NE(two_unconfirmed.find("without confirmation"),
            std::string_view::npos);
  EXPECT_NE(one_unconfirmed.find("without confirmation"),
            std::string_view::npos);
}

}  // namespace
