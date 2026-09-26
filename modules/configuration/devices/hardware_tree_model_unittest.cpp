#include "configuration/devices/hardware_tree_model.h"

#include "address_space/test/test_scada_node_states.h"
#include "base/test/test_executor.h"
#include "common/node_state.h"
#include "configuration/tree/node_service_tree_impl.h"
#include "model/data_items_node_ids.h"
#include "model/devices_node_ids.h"
#include "node_service/static/static_node_service.h"
#include "timed_data/timed_data_service_fake.h"

#include <gtest/gtest.h>

namespace {

// A destination device and the two kinds of child it has: a transmission rule
// and a device under it. Both are organized, which is the whole difficulty —
// the reference says nothing about which is which, so only the type does.
const scada::NodeId kLinkId{5001, 1};
const scada::NodeId kDestinationId{5002, 1};
const scada::NodeId kRuleId{5003, 1};
const scada::NodeId kChildDeviceId{5004, 1};
// The signal the rule transmits: a peer elsewhere in the address space, named
// by the rule's SourceNode property VALUE rather than by a reference.
const scada::NodeId kSourceId{5005, 1};
// A second rule under the same destination with no source configured yet.
const scada::NodeId kUnsourcedRuleId{5006, 1};

class HardwareTreeModelTest : public ::testing::Test {
 protected:
  HardwareTreeModelTest()
      : node_service_tree_factory_{[](NodeServiceTreeImplContext&& context) {
          return std::make_unique<NodeServiceTreeImpl>(std::move(context));
        }} {}

  void SetUp() override {
    node_service_.AddAll(GetScadaNodeStates());

    node_service_.Add(
        scada::NodeState{.node_id = kLinkId,
                         .node_class = scada::NodeClass::Object,
                         .type_definition_id = scada::devices::id::LinkType,
                         .parent_id = scada::devices::id::Devices,
                         .reference_type_id = scada::id::Organizes});
    node_service_.Add(
        scada::NodeState{.node_id = kDestinationId,
                         .node_class = scada::NodeClass::Object,
                         .type_definition_id = scada::devices::id::DeviceType,
                         .parent_id = kLinkId,
                         .reference_type_id = scada::id::Organizes});
    // The rule carries the per-protocol SUBTYPE, which is how a real server
    // types it — and is the case the leaf list has to catch without walking a
    // supertype chain nothing has fetched yet.
    //
    // Its DisplayName is the one the tier synthesises for a rule row that has
    // none — `<namespace name>.<id>` — which names the row without
    // identifying the rule.
    node_service_.Add(scada::NodeState{
        .node_id = kRuleId,
        .node_class = scada::NodeClass::Object,
        .type_definition_id = scada::devices::id::ModbusTransmissionItemType,
        .parent_id = kDestinationId,
        .reference_type_id = scada::id::Organizes,
        .attributes = {.display_name = u"IEC_TRANSMIT.1"},
        .properties = {
            {scada::devices::id::TransmissionItemType_SourceNode, kSourceId},
            {scada::devices::id::TransmissionItemType_Address,
             static_cast<scada::Int32>(2001)}}});
    node_service_.Add(scada::NodeState{
        .node_id = kUnsourcedRuleId,
        .node_class = scada::NodeClass::Object,
        .type_definition_id = scada::devices::id::ModbusTransmissionItemType,
        .parent_id = kDestinationId,
        .reference_type_id = scada::id::Organizes,
        .attributes = {.display_name = u"IEC_TRANSMIT.2"},
        .properties = {{scada::devices::id::TransmissionItemType_Address,
                        static_cast<scada::Int32>(2002)}}});
    node_service_.Add(scada::NodeState{
        .node_id = kSourceId,
        .node_class = scada::NodeClass::Variable,
        .type_definition_id = scada::data_items::id::AnalogItemType,
        .attributes = {.display_name = u"Ua"}});
    node_service_.Add(
        scada::NodeState{.node_id = kChildDeviceId,
                         .node_class = scada::NodeClass::Object,
                         .type_definition_id = scada::devices::id::DeviceType,
                         .parent_id = kDestinationId,
                         .reference_type_id = scada::id::Organizes,
                         .attributes = {.display_name = u"Feeder 7"}});

    model_ = std::make_unique<HardwareTreeModel>(HardwareTreeModelContext{
        .executor_ = executor_,
        .node_service_ = node_service_,
        .timed_data_service_ = timed_data_service_,
        .node_service_tree_factory_ = node_service_tree_factory_,
    });
    model_->Init();
  }

  // The row for `node_id`, with every level above it expanded — the tree only
  // creates a row once its parent's children have been fetched.
  ConfigurationTreeNode* RowFor(const scada::NodeId& node_id) {
    FetchAll(model_->root());
    return model_->FindFirstTreeNode(node_id);
  }

  void FetchAll(ConfigurationTreeNode* node) {
    if (!node)
      return;
    if (node->CanFetchMore())
      node->FetchMore();
    executor_.Poll();
    for (int i = 0; i < node->GetChildCount(); ++i)
      FetchAll(&node->GetChild(i));
  }

  TestExecutor executor_;
  StaticNodeService node_service_;
  FakeTimedDataService timed_data_service_;
  NodeServiceTreeFactory node_service_tree_factory_;
  std::unique_ptr<HardwareTreeModel> model_;
};

// A transmission rule is a row, never a branch. Its only children are the
// `Address` and `SourceNode` properties, which hang off HasProperty and so are
// outside this tree's reference filter — the expander opened onto nothing.
TEST_F(HardwareTreeModelTest, TransmissionRuleIsALeaf) {
  ConfigurationTreeNode* rule = RowFor(kRuleId);

  ASSERT_NE(rule, nullptr);
  EXPECT_FALSE(rule->HasChildren());
}

// The negative control, and the reason the leaf list names types rather than
// "everything under a device": a device organized under another device keeps
// its expander, so the rule cannot be hidden by making the level a leaf.
TEST_F(HardwareTreeModelTest, DeviceUnderADeviceKeepsItsExpander) {
  ConfigurationTreeNode* child_device = RowFor(kChildDeviceId);

  ASSERT_NE(child_device, nullptr);
  EXPECT_TRUE(child_device->HasChildren());
}

// The destination itself is still a branch — it is what holds the rules.
TEST_F(HardwareTreeModelTest, DestinationDeviceKeepsItsExpander) {
  ConfigurationTreeNode* destination = RowFor(kDestinationId);

  ASSERT_NE(destination, nullptr);
  EXPECT_TRUE(destination->HasChildren());
}

// Backlog 792: a rule row reads the way the rules grid and both clients' rule
// inspectors head it — its source signal and the address it is sent under —
// rather than the synthesised `IEC_TRANSMIT.n` the tier gives every rule.
TEST_F(HardwareTreeModelTest, TransmissionRuleIsLabelledBySourceAndAddress) {
  ConfigurationTreeNode* rule = RowFor(kRuleId);
  executor_.Poll();

  ASSERT_NE(rule, nullptr);
  EXPECT_EQ(rule->GetText(0), u"Ua → 2001");
}

// A rule with no source is an ordinary half-configured rule. It still gets
// the summary — an em dash where the source goes — so it does not fall back
// to a synthesised name that looks like every other rule's.
TEST_F(HardwareTreeModelTest, UnsourcedRuleShowsADashForTheSource) {
  ConfigurationTreeNode* rule = RowFor(kUnsourcedRuleId);
  executor_.Poll();

  ASSERT_NE(rule, nullptr);
  EXPECT_EQ(rule->GetText(0), u"— → 2002");
}

// The negative control: only a rule is relabelled. A device keeps its own
// DisplayName, which is what identifies it.
TEST_F(HardwareTreeModelTest, DeviceKeepsItsDisplayName) {
  ConfigurationTreeNode* child_device = RowFor(kChildDeviceId);
  executor_.Poll();

  ASSERT_NE(child_device, nullptr);
  EXPECT_EQ(child_device->GetText(0), u"Feeder 7");
}

// And the rule is still a ROW: making it a leaf must not drop it from the
// tree, which is what a `type_definition_ids_` change would have done.
TEST_F(HardwareTreeModelTest, TransmissionRuleIsStillShown) {
  EXPECT_NE(RowFor(kRuleId), nullptr);
}

}  // namespace
