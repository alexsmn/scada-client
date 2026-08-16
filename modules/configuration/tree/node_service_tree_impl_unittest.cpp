#include "configuration/tree/node_service_tree_impl.h"

#include "base/test/test_executor.h"
#include "common/node_state.h"
#include "node_service/test/fake_node_service.h"
#include "scada/standard_node_ids.h"

#include <gtest/gtest.h>

namespace {

// Stands in for `scada::data_items::id::DataItemType` and its two subtypes.
// The generated model headers are not visible to this module, and the leaf
// mechanism is type-agnostic, so the shape is what matters: a base type, a
// subtype of it (the TS/TIT case), and an unrelated type.
const scada::NodeId kItemType{500, 1};
const scada::NodeId kDiscreteItemType{501, 1};
const scada::NodeId kGroupType{502, 1};

const scada::NodeId kRootId{1, 2};
const scada::NodeId kGroupId{2, 2};
const scada::NodeId kItemId{3, 2};

class NodeServiceTreeImplTest : public testing::Test {
 public:
  NodeServiceTreeImplTest() {
    // The type graph: kDiscreteItemType is a subtype of kItemType, so a leaf
    // list naming the base has to catch an instance of the subtype.
    node_service_.Add(scada::NodeState{
        .node_id = kItemType, .node_class = scada::NodeClass::VariableType});
    node_service_.Add(
        scada::NodeState{.node_id = kDiscreteItemType,
                         .node_class = scada::NodeClass::VariableType,
                         .supertype_id = kItemType});
    node_service_.Add(scada::NodeState{
        .node_id = kGroupType, .node_class = scada::NodeClass::ObjectType});
  }

  // Builds the tree with |leaf_type_definition_ids| and the Organizes-only
  // reference filter the object tree uses.
  std::unique_ptr<NodeServiceTreeImpl> MakeTree(
      std::vector<scada::NodeId> leaf_type_definition_ids) {
    return std::make_unique<NodeServiceTreeImpl>(NodeServiceTreeImplContext{
        .executor_ = executor_,
        .node_service_ = node_service_,
        .root_node_ = NodeRef{kRootId, &node_service_},
        .reference_filter_ = {{scada::id::Organizes, true}},
        .leaf_type_definition_ids_ = std::move(leaf_type_definition_ids)});
  }

  FakeNodeService node_service_;
  TestExecutor executor_;
};

// The regression this file exists for. A TS/TIT row is an *instance* of a
// subtype of DataItemType; the leaf check used to walk the instance's own
// HasSubtype chain, which is empty, so the row always claimed children and
// rendered an expander that opened onto nothing.
TEST_F(NodeServiceTreeImplTest, InstanceOfALeafSubtypeHasNoChildren) {
  const NodeRef item = node_service_.Add(
      scada::NodeState{.node_id = kItemId,
                       .node_class = scada::NodeClass::Variable,
                       .type_definition_id = kDiscreteItemType,
                       .parent_id = kGroupId,
                       .reference_type_id = scada::id::Organizes});

  auto tree = MakeTree({kItemType});

  EXPECT_FALSE(tree->HasChildren(item));
  EXPECT_TRUE(tree->GetChildren(item).empty());
}

// The exact leaf type must match too, not only a subtype of it.
TEST_F(NodeServiceTreeImplTest, InstanceOfTheLeafTypeItselfHasNoChildren) {
  const NodeRef item = node_service_.Add(
      scada::NodeState{.node_id = kItemId,
                       .node_class = scada::NodeClass::Variable,
                       .type_definition_id = kItemType,
                       .parent_id = kGroupId,
                       .reference_type_id = scada::id::Organizes});

  EXPECT_FALSE(MakeTree({kItemType})->HasChildren(item));
}

// A container must keep its expander, so the fix cannot be "no node ever has
// children".
TEST_F(NodeServiceTreeImplTest, InstanceOfANonLeafTypeStillHasChildren) {
  const NodeRef group = node_service_.Add(
      scada::NodeState{.node_id = kGroupId,
                       .node_class = scada::NodeClass::Object,
                       .type_definition_id = kGroupType,
                       .parent_id = kRootId,
                       .reference_type_id = scada::id::Organizes});
  node_service_.Add(
      scada::NodeState{.node_id = kItemId,
                       .node_class = scada::NodeClass::Variable,
                       .type_definition_id = kDiscreteItemType,
                       .parent_id = kGroupId,
                       .reference_type_id = scada::id::Organizes});

  auto tree = MakeTree({kItemType});

  EXPECT_TRUE(tree->HasChildren(group));
  EXPECT_EQ(tree->GetChildren(group).size(), 1u);
}

// With no leaf list configured — the nodes view and the hardware tree — every
// node stays expandable.
TEST_F(NodeServiceTreeImplTest, EmptyLeafListLeavesEveryNodeExpandable) {
  const NodeRef item = node_service_.Add(
      scada::NodeState{.node_id = kItemId,
                       .node_class = scada::NodeClass::Variable,
                       .type_definition_id = kDiscreteItemType,
                       .parent_id = kGroupId,
                       .reference_type_id = scada::id::Organizes});

  EXPECT_TRUE(MakeTree({})->HasChildren(item));
}

}  // namespace
