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
                     std::optional<std::u16string> output = std::nullopt) {
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

}  // namespace
