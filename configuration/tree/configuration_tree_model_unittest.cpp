#include "configuration/tree/configuration_tree_model.h"

#include "aui/translation.h"
#include "configuration/tree/node_service_tree_mock.h"
#include "scada/standard_node_ids.h"
#include "node_service/node_model_mock.h"

#include <gmock/gmock.h>

using namespace testing;

namespace {

NodeRef MakeTestNodeRef(const scada::NodeId& node_id) {
  auto node_model = std::make_shared<NiceMock<MockNodeModel>>();
  ON_CALL(*node_model, GetAttribute(scada::AttributeId::NodeId))
      .WillByDefault(Return(node_id));
  ON_CALL(*node_model, GetFetchStatus())
      .WillByDefault(Return(NodeFetchStatus::NodeAndChildren()));
  return node_model;
}

std::shared_ptr<NiceMock<MockNodeModel>> MakeTestNodeModel(
    const scada::NodeId& node_id) {
  auto node_model = std::make_shared<NiceMock<MockNodeModel>>();
  ON_CALL(*node_model, GetAttribute(scada::AttributeId::NodeId))
      .WillByDefault(Return(node_id));
  ON_CALL(*node_model, GetFetchStatus())
      .WillByDefault(Return(NodeFetchStatus::NodeAndChildren()));
  return node_model;
}

}  // namespace

class ConfigurationTreeModelTest : public Test {
 public:
  void InitModel(std::unique_ptr<MockNodeServiceTree> node_service_tree);
  void InitModel(std::unique_ptr<MockNodeServiceTree> node_service_tree,
                 NodeRef root_node);

  const NodeRef root_node_ = MakeTestNodeRef(scada::id::RootFolder);

  MockNodeServiceTree* node_service_tree_ = nullptr;
  NodeServiceTree::Observer* observer_ = nullptr;
  std::unique_ptr<ConfigurationTreeModel> model_;

  inline static const scada::NodeId kNodeId1{1, 1};
  inline static const scada::NodeId kNodeId2{2, 1};
  inline static const scada::NodeId kNodeId3{3, 1};
};

void ConfigurationTreeModelTest::InitModel(
    std::unique_ptr<MockNodeServiceTree> node_service_tree) {
  InitModel(std::move(node_service_tree), root_node_);
}

void ConfigurationTreeModelTest::InitModel(
    std::unique_ptr<MockNodeServiceTree> node_service_tree,
    NodeRef root_node) {
  node_service_tree_ = node_service_tree.get();

  EXPECT_CALL(*node_service_tree_, SetObserver(_))
      .WillOnce([this](NodeServiceTree::Observer* observer) {
        observer_ = observer;
      });

  EXPECT_CALL(*node_service_tree_, GetRoot()).WillOnce(Return(root_node));

  ON_CALL(*node_service_tree_, GetChildren(_))
      .WillByDefault(Return(std::vector<NodeServiceTree::ChildRef>{}));

  model_ = std::make_unique<ConfigurationTreeModel>(
      ConfigurationTreeModelContext{std::move(node_service_tree)});
  model_->Init();

  EXPECT_TRUE(model_->root_node() == root_node);
}

TEST_F(ConfigurationTreeModelTest, PrefetchedChildrenAreAvailable) {
  auto node_service_tree = std::make_unique<NiceMock<MockNodeServiceTree>>();

  // Only the root node prefetches its direct children. Grandchildren load
  // lazily through `FetchMore`; see the construction-doesn't-recurse test
  // below for the reason.
  // WARNING: Cannot use the equality matcher for the `NodeRef` parameter as it
  // seems to bring a deadlock.
  EXPECT_CALL(*node_service_tree, GetChildren(_))
      .WillOnce(Return(std::vector<NodeServiceTree::ChildRef>{
          {.reference_type_id = scada::id::Organizes,
           .child_node = MakeTestNodeRef(kNodeId1)},
          {.reference_type_id = scada::id::Organizes,
           .child_node = MakeTestNodeRef(kNodeId2)},
          {.reference_type_id = scada::id::Organizes,
           .child_node = MakeTestNodeRef(kNodeId3)}}));

  InitModel(std::move(node_service_tree));

  auto* root = model_->GetRoot();
  EXPECT_EQ(3, model_->GetChildCount(root));
}

// Regression: `ConfigurationTreeNode`'s ctor used to invoke `AddChildren()`,
// which called the ctor of every child, which again called `AddChildren()`,
// and so on. When the address space starts pre-populated (e.g. the
// screenshot generator wired over AddressSpaceImpl3) every node already
// has children loaded, so the chain walked the entire tree synchronously
// and exploded the stack. The fix restricts the eager prefetch to the
// root node — grandchildren stay lazy and only load via `FetchMore`.
//
// This test configures a mock tree where every node reports one child, ad
// infinitum. The old implementation would recurse forever and trip the
// `gmock` call limit (or, in a release build, the process stack); the new
// one stops after the root's single prefetch.
TEST_F(ConfigurationTreeModelTest, ConstructionDoesNotRecurseIntoGrandchildren) {
  auto node_service_tree = std::make_unique<NiceMock<MockNodeServiceTree>>();

  // Every `GetChildren(...)` call returns a single child whose id is one
  // past the caller's — a bottomless chain. Only the root's prefetch must
  // run; if the ctor recursed into the child the mock would be called
  // more than once and `.Times(1)` would fail.
  EXPECT_CALL(*node_service_tree, GetChildren(_))
      .Times(1)
      .WillRepeatedly([](const NodeRef& node) {
        const auto child_id =
            scada::NodeId{node.node_id().numeric_id() + 1, 1};
        return std::vector<NodeServiceTree::ChildRef>{
            {.reference_type_id = scada::id::Organizes,
             .child_node = MakeTestNodeRef(child_id)}};
      });

  InitModel(std::move(node_service_tree));

  auto* root = model_->GetRoot();
  EXPECT_EQ(1, model_->GetChildCount(root));
}

TEST_F(ConfigurationTreeModelTest, CreatingChildrenDoesNotFetchNodesInCtor) {
  auto node_service_tree = std::make_unique<NiceMock<MockNodeServiceTree>>();
  auto child_model = MakeTestNodeModel(kNodeId1);

  EXPECT_CALL(*child_model, Fetch(_, _)).Times(0);

  EXPECT_CALL(*node_service_tree, GetChildren(_))
      .WillOnce(Return(std::vector<NodeServiceTree::ChildRef>{
          {.reference_type_id = scada::id::Organizes, .child_node = child_model}
      }));

  InitModel(std::move(node_service_tree));

  auto* root = model_->GetRoot();
  ASSERT_EQ(1, model_->GetChildCount(root));
  EXPECT_EQ(static_cast<ConfigurationTreeNode*>(model_->GetChild(root, 0))
                ->node()
                .node_id(),
            kNodeId1);
}

TEST_F(ConfigurationTreeModelTest,
       ChildrenChangedDoesNotPopulateCollapsedDescendants) {
  auto node_service_tree = std::make_unique<NiceMock<MockNodeServiceTree>>();

  EXPECT_CALL(*node_service_tree, GetChildren(_))
      .Times(1)
      .WillOnce(Return(std::vector<NodeServiceTree::ChildRef>{
          {.reference_type_id = scada::id::Organizes,
           .child_node = MakeTestNodeRef(kNodeId1)}}));

  InitModel(std::move(node_service_tree));
  ASSERT_NE(observer_, nullptr);

  observer_->OnNodeChildrenChanged(kNodeId1);

  auto* root = model_->GetRoot();
  ASSERT_EQ(1, model_->GetChildCount(root));
  EXPECT_EQ(
      static_cast<ConfigurationTreeNode*>(model_->GetChild(root, 0))
          ->GetChildCount(),
      0);
}

TEST_F(ConfigurationTreeModelTest,
       FetchMoreAddsChildrenWithoutObserverReferenceChange) {
  auto node_service_tree = std::make_unique<NiceMock<MockNodeServiceTree>>();
  auto child_model = MakeTestNodeModel(kNodeId1);
  bool children_fetched = false;

  ON_CALL(*child_model, GetFetchStatus())
      .WillByDefault([&] {
        return children_fetched ? NodeFetchStatus::NodeAndChildren()
                                : NodeFetchStatus::NodeOnly();
      });

  EXPECT_CALL(*child_model, Fetch(NodeFetchStatus::NodeAndChildren(), _))
      .WillOnce([&](const NodeFetchStatus&, const NodeModel::FetchCallback& cb) {
        children_fetched = true;
        cb();
      });

  EXPECT_CALL(*node_service_tree, GetChildren(_))
      .WillOnce(Return(std::vector<NodeServiceTree::ChildRef>{
          {.reference_type_id = scada::id::Organizes, .child_node = child_model}
      }))
      .WillOnce(Return(std::vector<NodeServiceTree::ChildRef>{
          {.reference_type_id = scada::id::Organizes,
           .child_node = MakeTestNodeRef(kNodeId2)}}));

  InitModel(std::move(node_service_tree));

  auto* root = model_->GetRoot();
  ASSERT_EQ(1, model_->GetChildCount(root));

  auto* child =
      static_cast<ConfigurationTreeNode*>(model_->GetChild(root, 0));
  EXPECT_EQ(0, child->GetChildCount());

  child->FetchMore();

  EXPECT_EQ(1, child->GetChildCount());
  EXPECT_EQ(child->GetChild(0).node().node_id(), kNodeId2);
}

TEST_F(ConfigurationTreeModelTest,
       FetchMoreShowsTranslatedLoadingSuffixWithoutDotsWhilePending) {
  auto node_service_tree = std::make_unique<NiceMock<MockNodeServiceTree>>();
  auto child_model = MakeTestNodeModel(kNodeId1);

  ON_CALL(*child_model, GetFetchStatus())
      .WillByDefault(Return(NodeFetchStatus::NodeOnly()));
  ON_CALL(*child_model, GetAttribute(scada::AttributeId::DisplayName))
      .WillByDefault(Return(scada::LocalizedText{u"Loading node"}));

  EXPECT_CALL(*child_model, Fetch(NodeFetchStatus::NodeAndChildren(), _))
      .WillOnce([](const NodeFetchStatus&, const NodeModel::FetchCallback&) {});

  EXPECT_CALL(*node_service_tree, GetChildren(_))
      .WillOnce(Return(std::vector<NodeServiceTree::ChildRef>{
          {.reference_type_id = scada::id::Organizes, .child_node = child_model}
      }));

  InitModel(std::move(node_service_tree));

  auto* root = model_->GetRoot();
  ASSERT_EQ(1, model_->GetChildCount(root));

  auto* child =
      static_cast<ConfigurationTreeNode*>(model_->GetChild(root, 0));
  child->FetchMore();

  EXPECT_EQ(child->GetText(0),
            u"Loading node [" + Translate("Loading") + u"]");
}

TEST_F(ConfigurationTreeModelTest, RootFetchesChildrenWhenNotPrefetched) {
  auto node_service_tree = std::make_unique<NiceMock<MockNodeServiceTree>>();
  auto root_model = MakeTestNodeModel(scada::id::RootFolder);
  auto child_model = MakeTestNodeModel(kNodeId1);
  bool root_children_fetched = false;

  ON_CALL(*root_model, GetFetchStatus())
      .WillByDefault([&] {
        return root_children_fetched ? NodeFetchStatus::NodeAndChildren()
                                     : NodeFetchStatus::NodeOnly();
      });

  EXPECT_CALL(*root_model, Fetch(NodeFetchStatus::NodeAndChildren(), _))
      .WillOnce([&](const NodeFetchStatus&, const NodeModel::FetchCallback& cb) {
        root_children_fetched = true;
        cb();
      });

  EXPECT_CALL(*node_service_tree, GetChildren(_))
      .WillOnce(Return(std::vector<NodeServiceTree::ChildRef>{
          {.reference_type_id = scada::id::Organizes, .child_node = child_model}
      }));

  InitModel(std::move(node_service_tree), root_model);

  auto* root = model_->GetRoot();
  ASSERT_EQ(1, model_->GetChildCount(root));
  EXPECT_EQ(static_cast<ConfigurationTreeNode*>(model_->GetChild(root, 0))
                ->node()
                .node_id(),
            kNodeId1);
}

TEST_F(ConfigurationTreeModelTest,
       DelayedFetchMoreCallbackAfterModelDestructionIsIgnored) {
  auto node_service_tree = std::make_unique<NiceMock<MockNodeServiceTree>>();
  auto child_model = MakeTestNodeModel(kNodeId1);
  NodeModel::FetchCallback delayed_callback;

  ON_CALL(*child_model, GetFetchStatus())
      .WillByDefault(Return(NodeFetchStatus::NodeOnly()));

  EXPECT_CALL(*child_model, Fetch(NodeFetchStatus::NodeAndChildren(), _))
      .WillOnce([&](const NodeFetchStatus&, const NodeModel::FetchCallback& cb) {
        delayed_callback = cb;
      });

  EXPECT_CALL(*node_service_tree, GetChildren(_))
      .WillOnce(Return(std::vector<NodeServiceTree::ChildRef>{
          {.reference_type_id = scada::id::Organizes, .child_node = child_model}
      }));

  InitModel(std::move(node_service_tree));

  auto* root = model_->GetRoot();
  ASSERT_EQ(1, model_->GetChildCount(root));

  auto* child =
      static_cast<ConfigurationTreeNode*>(model_->GetChild(root, 0));
  child->FetchMore();
  ASSERT_TRUE(static_cast<bool>(delayed_callback));

  model_.reset();

  EXPECT_NO_THROW(delayed_callback());
}
