#include "configuration/tree/node_service_tree_impl.h"

#include "base/test/test_executor.h"
#include "common/node_state.h"
#include "node_service/node_util.h"
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
// A type the fake service never learns about, standing in for a type node that
// has not been fetched yet: `supertype()` on it yields nothing, so the
// supertype walk cannot reach kItemType.
const scada::NodeId kUnfetchedItemType{503, 1};

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

  // Builds the tree with |leaf_type_definition_ids| and, by default, the
  // Organizes-only reference filter the object and filesystem trees use.
  // |reference_filter| is a parameter because the NodeClass leaf rule is a
  // statement about Organizes: the hardware tree (Organizes + HasComponent) and
  // the nodes view (HierarchicalReferences) must not get it.
  std::unique_ptr<NodeServiceTreeImpl> MakeTree(
      std::vector<scada::NodeId> leaf_type_definition_ids,
      NodeServiceTreeImplContext::ReferenceFilter reference_filter = {
          {scada::id::Organizes, true}}) {
    return std::make_unique<NodeServiceTreeImpl>(NodeServiceTreeImplContext{
        .executor_ = executor_,
        .node_service_ = node_service_,
        .root_node_ = NodeRef{kRootId, &node_service_},
        .reference_filter_ = std::move(reference_filter),
        .leaf_type_definition_ids_ = std::move(leaf_type_definition_ids)});
  }

  // A TS/TIT row: a Variable instance of a subtype of the leaf type, organized
  // under a group.
  NodeRef AddItem() {
    return node_service_.Add(
        scada::NodeState{.node_id = kItemId,
                         .node_class = scada::NodeClass::Variable,
                         .type_definition_id = kDiscreteItemType,
                         .parent_id = kGroupId,
                         .reference_type_id = scada::id::Organizes});
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

// With no leaf list configured, a container stays expandable — the leaf
// mechanism must not degrade into "nothing has children".
TEST_F(NodeServiceTreeImplTest, EmptyLeafListLeavesContainersExpandable) {
  const NodeRef group = node_service_.Add(
      scada::NodeState{.node_id = kGroupId,
                       .node_class = scada::NodeClass::Object,
                       .type_definition_id = kGroupType,
                       .parent_id = kRootId,
                       .reference_type_id = scada::id::Organizes});

  EXPECT_TRUE(MakeTree({})->HasChildren(group));
}

// The rule the expander bug turns on. `HasChildren` is asked the moment the row
// is created, and at that moment the instance is fetched but its *type* is not
// — so the supertype walk cannot yet see that kDiscreteItemType descends from
// kItemType. Dropping the type from the service reproduces exactly that state:
// the answer must still be "leaf", from the NodeClass alone.
TEST_F(NodeServiceTreeImplTest, VariableIsALeafBeforeItsTypeChainResolves) {
  const NodeRef item = node_service_.Add(
      scada::NodeState{.node_id = kItemId,
                       .node_class = scada::NodeClass::Variable,
                       .type_definition_id = kUnfetchedItemType,
                       .parent_id = kGroupId,
                       .reference_type_id = scada::id::Organizes});

  // Precondition: the type system genuinely cannot answer here.
  EXPECT_FALSE(IsInstanceOf(item, kItemType));

  EXPECT_FALSE(MakeTree({kItemType})->HasChildren(item));
}

// ...and it holds with no leaf list at all, because it is a fact about the
// reference type rather than about the configured types. This is what makes the
// filesystem tree (leaf type FileType, also a VariableType) correct on the
// first paint too.
TEST_F(NodeServiceTreeImplTest, VariableIsALeafUnderOrganizesWithNoLeafList) {
  EXPECT_FALSE(MakeTree({})->HasChildren(AddItem()));
}

// The rule must not escape its premise. Under HierarchicalReferences a Variable
// legitimately has children — its own properties — which is what the nodes view
// shows, so the NodeClass shortcut must not fire.
TEST_F(NodeServiceTreeImplTest, VariableKeepsChildrenUnderHierarchicalFilter) {
  const NodeRef item = AddItem();

  EXPECT_TRUE(MakeTree({}, {{scada::id::HierarchicalReferences, true}})
                  ->HasChildren(item));
}

// HasComponent admits a DataVariable as its SourceNode (Part 3 §7.7), so the
// hardware tree's filter disqualifies the shortcut even though Organizes is in
// it.
TEST_F(NodeServiceTreeImplTest,
       VariableKeepsChildrenWhenFilterAddsHasComponent) {
  const NodeRef item = AddItem();

  EXPECT_TRUE(MakeTree({}, {{scada::id::Organizes, true},
                            {scada::id::HasComponent, true}})
                  ->HasChildren(item));
}

// An inverse Organizes entry follows the reference to whatever organizes the
// node, and Part 3 §7.11 puts no NodeClass constraint on an Organizes target —
// so a Variable is not a leaf there.
TEST_F(NodeServiceTreeImplTest, VariableKeepsChildrenUnderInverseOrganizes) {
  const NodeRef item = AddItem();

  EXPECT_TRUE(MakeTree({}, {{scada::id::Organizes, false}})->HasChildren(item));
}

// An Object under the same filter is untouched by the rule: Part 3 §7.11 names
// Object as a permitted SourceNode.
TEST_F(NodeServiceTreeImplTest, ObjectKeepsChildrenUnderOrganizes) {
  const NodeRef group = node_service_.Add(
      scada::NodeState{.node_id = kGroupId,
                       .node_class = scada::NodeClass::Object,
                       .type_definition_id = kGroupType,
                       .parent_id = kRootId,
                       .reference_type_id = scada::id::Organizes});

  EXPECT_TRUE(MakeTree({})->HasChildren(group));
}

}  // namespace
