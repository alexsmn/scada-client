#include "configuration/tree/configuration_tree_model.h"

#include "configuration/tree/configuration_tree_node.h"
#include "resources/icon_strips.h"

#include <filesystem>
#include <fstream>
#include <iterator>

#include "aui/translation.h"
#include "base/async_completion.h"
#include "base/test/awaitable_test.h"
#include "base/test/test_executor.h"
#include "common/node_state.h"
#include "configuration/tree/node_service_tree_mock.h"
#include "node_service/test/fake_node_service.h"
#include "scada/standard_node_ids.h"

#include <gmock/gmock.h>

#include <optional>

using namespace testing;

class ConfigurationTreeModelTest : public Test {
 public:
  void InitModel(std::unique_ptr<MockNodeServiceTree> node_service_tree);
  void InitModel(std::unique_ptr<MockNodeServiceTree> node_service_tree,
                 NodeRef root_node);

  // Registers a node and returns a cursor to it. Nodes default to fully
  // fetched; tests that need a partially-loaded node override it with
  // `node_service_.SetFetchStatus`. The backing service (|node_service_|)
  // outlives every cursor it hands out.
  NodeRef MakeTestNodeRef(const scada::NodeId& node_id) {
    return node_service_.Add(scada::NodeState{.node_id = node_id});
  }

  FakeNodeService node_service_;
  const NodeRef root_node_ = MakeTestNodeRef(scada::id::RootFolder);

  TestExecutor executor_;
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
      ConfigurationTreeModelContext{executor_, std::move(node_service_tree)});
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
// screenshot generator wired over ScadaTestAddressSpace) every node already
// has children loaded, so the chain walked the entire tree synchronously
// and exploded the stack. The fix restricts the eager prefetch to the
// root node — grandchildren stay lazy and only load via `FetchMore`.
//
// This test configures a mock tree where every node reports one child, ad
// infinitum. The old implementation would recurse forever and trip the
// `gmock` call limit (or, in a release build, the process stack); the new
// one stops after the root's single prefetch.
TEST_F(ConfigurationTreeModelTest,
       ConstructionDoesNotRecurseIntoGrandchildren) {
  auto node_service_tree = std::make_unique<NiceMock<MockNodeServiceTree>>();

  // Every `GetChildren(...)` call returns a single child whose id is one
  // past the caller's — a bottomless chain. Only the root's prefetch must
  // run; if the ctor recursed into the child the mock would be called
  // more than once and `.Times(1)` would fail.
  EXPECT_CALL(*node_service_tree, GetChildren(_))
      .Times(1)
      .WillRepeatedly([this](const NodeRef& node) {
        const auto child_id = scada::NodeId{node.node_id().numeric_id() + 1, 1};
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
  EXPECT_CALL(*node_service_tree, GetChildren(_))
      .WillOnce(Return(std::vector<NodeServiceTree::ChildRef>{
          {.reference_type_id = scada::id::Organizes,
           .child_node = MakeTestNodeRef(kNodeId1)}}));

  InitModel(std::move(node_service_tree));

  // Constructing the child node must not have requested a fetch.
  EXPECT_TRUE(node_service_.fetch_requests(kNodeId1).empty());

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
  EXPECT_EQ(static_cast<ConfigurationTreeNode*>(model_->GetChild(root, 0))
                ->GetChildCount(),
            0);
}

TEST_F(ConfigurationTreeModelTest,
       FetchMoreAddsChildrenWithoutObserverReferenceChange) {
  auto node_service_tree = std::make_unique<NiceMock<MockNodeServiceTree>>();
  // The child starts with only its own attributes loaded; fetching completes
  // its children.
  const NodeRef child_node = MakeTestNodeRef(kNodeId1);
  node_service_.SetFetchStatus(kNodeId1, NodeFetchStatus::NodeOnly);
  node_service_.SetFetchHandler(
      kNodeId1, [&](const NodeFetchStatus&) -> Awaitable<void> {
        node_service_.SetFetchStatus(kNodeId1,
                                     NodeFetchStatus::NodeAndChildren);
        co_return;
      });

  EXPECT_CALL(*node_service_tree, GetChildren(_))
      .WillOnce(Return(std::vector<NodeServiceTree::ChildRef>{
          {.reference_type_id = scada::id::Organizes,
           .child_node = child_node}}))
      .WillOnce(Return(std::vector<NodeServiceTree::ChildRef>{
          {.reference_type_id = scada::id::Organizes,
           .child_node = MakeTestNodeRef(kNodeId2)}}));

  InitModel(std::move(node_service_tree));

  auto* root = model_->GetRoot();
  ASSERT_EQ(1, model_->GetChildCount(root));

  auto* child = static_cast<ConfigurationTreeNode*>(model_->GetChild(root, 0));
  EXPECT_EQ(0, child->GetChildCount());

  child->FetchMore();
  Drain(executor_);

  EXPECT_THAT(node_service_.fetch_requests(kNodeId1),
              Contains(NodeFetchStatus::NodeAndChildren));
  EXPECT_EQ(1, child->GetChildCount());
  EXPECT_EQ(child->GetChild(0).node().node_id(), kNodeId2);
}

TEST_F(ConfigurationTreeModelTest,
       FetchMoreShowsTranslatedLoadingSuffixWithoutDotsWhilePending) {
  auto node_service_tree = std::make_unique<NiceMock<MockNodeServiceTree>>();
  std::optional<scada::base::AsyncCompletion> delayed_completion;

  // The fetch never completes while the assertion below runs, so the node
  // stays in its "loading" state.
  const NodeRef child_node = node_service_.Add(scada::NodeState{
      .node_id = kNodeId1,
      .attributes = {.display_name = scada::LocalizedText{u"Loading node"}}});
  node_service_.SetFetchStatus(kNodeId1, NodeFetchStatus::NodeOnly);
  node_service_.SetFetchHandler(kNodeId1,
                                [&](const NodeFetchStatus&) -> Awaitable<void> {
                                  delayed_completion.emplace(executor_);
                                  co_await delayed_completion->Wait();
                                });

  EXPECT_CALL(*node_service_tree, GetChildren(_))
      .WillOnce(Return(std::vector<NodeServiceTree::ChildRef>{
          {.reference_type_id = scada::id::Organizes,
           .child_node = child_node}}))
      .WillOnce(Return(std::vector<NodeServiceTree::ChildRef>{}));

  InitModel(std::move(node_service_tree));

  auto* root = model_->GetRoot();
  ASSERT_EQ(1, model_->GetChildCount(root));

  auto* child = static_cast<ConfigurationTreeNode*>(model_->GetChild(root, 0));
  child->FetchMore();
  Drain(executor_);

  EXPECT_EQ(child->GetText(0), u"Loading node [" + Translate("Loading") + u"]");

  ASSERT_TRUE(delayed_completion.has_value());
  delayed_completion->Complete();
  Drain(executor_);
}

TEST_F(ConfigurationTreeModelTest, RootFetchesChildrenWhenNotPrefetched) {
  auto node_service_tree = std::make_unique<NiceMock<MockNodeServiceTree>>();
  // The root arrives without its children loaded, so the model must fetch it.
  const scada::NodeId root_id{scada::id::RootFolder};
  node_service_.SetFetchStatus(root_id, NodeFetchStatus::NodeOnly);
  node_service_.SetFetchHandler(
      root_id, [&](const NodeFetchStatus&) -> Awaitable<void> {
        node_service_.SetFetchStatus(root_id, NodeFetchStatus::NodeAndChildren);
        co_return;
      });

  EXPECT_CALL(*node_service_tree, GetChildren(_))
      .WillOnce(Return(std::vector<NodeServiceTree::ChildRef>{
          {.reference_type_id = scada::id::Organizes,
           .child_node = MakeTestNodeRef(kNodeId1)}}));

  InitModel(std::move(node_service_tree), root_node_);
  Drain(executor_);

  EXPECT_THAT(node_service_.fetch_requests(root_id),
              Contains(NodeFetchStatus::NodeAndChildren));

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
  std::optional<scada::base::AsyncCompletion> delayed_completion;

  // The fetch stays suspended until the test completes it by hand, after the
  // waiting tree node is gone.
  const NodeRef child_node = MakeTestNodeRef(kNodeId1);
  node_service_.SetFetchStatus(kNodeId1, NodeFetchStatus::NodeOnly);
  node_service_.SetFetchHandler(kNodeId1,
                                [&](const NodeFetchStatus&) -> Awaitable<void> {
                                  delayed_completion.emplace(executor_);
                                  co_await delayed_completion->Wait();
                                });

  EXPECT_CALL(*node_service_tree, GetChildren(_))
      .WillOnce(Return(std::vector<NodeServiceTree::ChildRef>{
          {.reference_type_id = scada::id::Organizes,
           .child_node = child_node}}));

  InitModel(std::move(node_service_tree));

  auto* root = model_->GetRoot();
  ASSERT_EQ(1, model_->GetChildCount(root));

  auto* child = static_cast<ConfigurationTreeNode*>(model_->GetChild(root, 0));
  child->FetchMore();
  Drain(executor_);
  ASSERT_TRUE(delayed_completion.has_value());

  model_.reset();

  EXPECT_NO_THROW(delayed_completion->Complete());
  Drain(executor_);
}

TEST_F(ConfigurationTreeModelTest,
       DelayedFetchMoreCallbackAfterTreeNodeRemovalIsIgnored) {
  auto node_service_tree = std::make_unique<NiceMock<MockNodeServiceTree>>();
  std::optional<scada::base::AsyncCompletion> delayed_completion;

  // The fetch stays suspended until the test completes it by hand, after the
  // waiting tree node is gone.
  const NodeRef child_node = MakeTestNodeRef(kNodeId1);
  node_service_.SetFetchStatus(kNodeId1, NodeFetchStatus::NodeOnly);
  node_service_.SetFetchHandler(kNodeId1,
                                [&](const NodeFetchStatus&) -> Awaitable<void> {
                                  delayed_completion.emplace(executor_);
                                  co_await delayed_completion->Wait();
                                });

  EXPECT_CALL(*node_service_tree, GetChildren(_))
      .WillOnce(Return(std::vector<NodeServiceTree::ChildRef>{
          {.reference_type_id = scada::id::Organizes,
           .child_node = child_node}}))
      .WillOnce(Return(std::vector<NodeServiceTree::ChildRef>{}));

  InitModel(std::move(node_service_tree));
  ASSERT_NE(observer_, nullptr);

  auto* root = model_->GetRoot();
  ASSERT_EQ(1, model_->GetChildCount(root));

  auto* child = static_cast<ConfigurationTreeNode*>(model_->GetChild(root, 0));
  child->FetchMore();
  Drain(executor_);
  ASSERT_TRUE(delayed_completion.has_value());

  observer_->OnNodeChildrenChanged(scada::id::RootFolder);
  ASSERT_EQ(0, model_->GetChildCount(root));

  EXPECT_NO_THROW(delayed_completion->Complete());
  Drain(executor_);
  EXPECT_EQ(0, model_->GetChildCount(root));
}

namespace {

// The IMAGE_* tile indices are protected; expose them the way the object-tree
// test already does.
struct GlyphIndices : ConfigurationTreeNode {
  using ConfigurationTreeNode::IMAGE_COUNT;
  using ConfigurationTreeNode::IMAGE_DEVICE;
  using ConfigurationTreeNode::IMAGE_DEVICE_DISABLED;
  using ConfigurationTreeNode::IMAGE_DEVICE_RUNNING;
  using ConfigurationTreeNode::IMAGE_DEVICE_STOPPED;
  using ConfigurationTreeNode::IMAGE_FOLDER;
  using ConfigurationTreeNode::IMAGE_SUBSYSTEM_RUNNING;
  using ConfigurationTreeNode::IMAGE_SUBSYSTEM_STOPPED;
};

// Task 117. The `[Loading]` suffix used to report only the *children* fetch,
// so a row whose own attributes had not arrived looked settled while showing
// nothing but a fallback display name. The ctor deliberately does not fetch
// (stack-overflow reason in its comment), so that state is routine, not rare.
TEST_F(ConfigurationTreeModelTest, UnfetchedNodeReportsLoading) {
  auto node_service_tree = std::make_unique<NiceMock<MockNodeServiceTree>>();
  EXPECT_CALL(*node_service_tree, GetChildren(_))
      .WillOnce(Return(std::vector<NodeServiceTree::ChildRef>{
          {.reference_type_id = scada::id::Organizes,
           .child_node = MakeTestNodeRef(kNodeId1)}}));

  node_service_.SetFetchStatus(kNodeId1, NodeFetchStatus::None);

  InitModel(std::move(node_service_tree));

  auto* child = model_->GetChild(model_->GetRoot(), 0);
  EXPECT_NE(model_->GetText(child, 0).find(u"[Loading]"), std::u16string::npos);
}

// The complement, so the suffix cannot simply be unconditional: a node that
// has been fetched, and whose children nobody asked for, is quiet.
TEST_F(ConfigurationTreeModelTest, FetchedNodeDoesNotReportLoading) {
  auto node_service_tree = std::make_unique<NiceMock<MockNodeServiceTree>>();
  EXPECT_CALL(*node_service_tree, GetChildren(_))
      .WillOnce(Return(std::vector<NodeServiceTree::ChildRef>{
          {.reference_type_id = scada::id::Organizes,
           .child_node = MakeTestNodeRef(kNodeId1)}}));

  InitModel(std::move(node_service_tree));

  auto* child = model_->GetChild(model_->GetRoot(), 0);
  EXPECT_EQ(model_->GetText(child, 0).find(u"[Loading]"), std::u16string::npos);
}

// The suffix clears once the node arrives, which is what makes it a progress
// report rather than a permanent label. A completed fetch notifies semantic
// change; the model turns that into a repaint.
TEST_F(ConfigurationTreeModelTest, LoadingClearsWhenTheNodeIsFetched) {
  auto node_service_tree = std::make_unique<NiceMock<MockNodeServiceTree>>();
  EXPECT_CALL(*node_service_tree, GetChildren(_))
      .WillOnce(Return(std::vector<NodeServiceTree::ChildRef>{
          {.reference_type_id = scada::id::Organizes,
           .child_node = MakeTestNodeRef(kNodeId1)}}));

  node_service_.SetFetchStatus(kNodeId1, NodeFetchStatus::None);

  InitModel(std::move(node_service_tree));

  auto* child = model_->GetChild(model_->GetRoot(), 0);
  ASSERT_NE(model_->GetText(child, 0).find(u"[Loading]"), std::u16string::npos);

  node_service_.SetFetchStatus(kNodeId1, NodeFetchStatus::Max);

  EXPECT_EQ(model_->GetText(child, 0).find(u"[Loading]"), std::u16string::npos);
}

// The root's own row is the one row that says "[Loading]" for the first level,
// and no ConfigurationTreeView draws it — Tree hides its root, so the dock
// title does not read twice (configuration_tree_view.cpp). That left the pane
// empty, with nothing to tell a slow server from an empty folder, for as long
// as the first level took. The placeholder row is the whole of that feedback.
TEST_F(ConfigurationTreeModelTest, PendingFirstLevelShowsAPlaceholderRow) {
  auto node_service_tree = std::make_unique<NiceMock<MockNodeServiceTree>>();
  std::optional<scada::base::AsyncCompletion> delayed_completion;

  // The root arrives without its children, and its fetch stays suspended until
  // the test releases it — the window this row exists for.
  const scada::NodeId root_id{scada::id::RootFolder};
  node_service_.SetFetchStatus(root_id, NodeFetchStatus::NodeOnly);
  node_service_.SetFetchHandler(
      root_id, [&](const NodeFetchStatus&) -> Awaitable<void> {
        delayed_completion.emplace(executor_);
        co_await delayed_completion->Wait();
        node_service_.SetFetchStatus(root_id, NodeFetchStatus::NodeAndChildren);
      });

  EXPECT_CALL(*node_service_tree, GetChildren(_))
      .WillOnce(Return(std::vector<NodeServiceTree::ChildRef>{
          {.reference_type_id = scada::id::Organizes,
           .child_node = MakeTestNodeRef(kNodeId1)}}));

  InitModel(std::move(node_service_tree), root_node_);
  Drain(executor_);

  auto* root = model_->GetRoot();
  ASSERT_EQ(1, model_->GetChildCount(root));
  auto* placeholder =
      static_cast<ConfigurationTreeNode*>(model_->GetChild(root, 0));
  ASSERT_TRUE(placeholder->IsLoadingPlaceholder());
  EXPECT_EQ(model_->GetText(placeholder, 0), Translate("Loading") + u"\u2026");

  ASSERT_TRUE(delayed_completion.has_value());
  delayed_completion->Complete();
  Drain(executor_);

  // Down again the moment the real rows go up, and the two are never both on
  // screen: one child, and it is the fetched one.
  ASSERT_EQ(1, model_->GetChildCount(root));
  auto* child = static_cast<ConfigurationTreeNode*>(model_->GetChild(root, 0));
  EXPECT_FALSE(child->IsLoadingPlaceholder());
  EXPECT_EQ(child->node().node_id(), kNodeId1);
}

// A root whose children were prefetched never waits, so it must not flash a
// row saying it did.
TEST_F(ConfigurationTreeModelTest, PrefetchedFirstLevelShowsNoPlaceholderRow) {
  auto node_service_tree = std::make_unique<NiceMock<MockNodeServiceTree>>();
  EXPECT_CALL(*node_service_tree, GetChildren(_))
      .WillOnce(Return(std::vector<NodeServiceTree::ChildRef>{
          {.reference_type_id = scada::id::Organizes,
           .child_node = MakeTestNodeRef(kNodeId1)}}));

  InitModel(std::move(node_service_tree));
  Drain(executor_);

  auto* root = model_->GetRoot();
  ASSERT_EQ(1, model_->GetChildCount(root));
  EXPECT_FALSE(static_cast<ConfigurationTreeNode*>(model_->GetChild(root, 0))
                   ->IsLoadingPlaceholder());
}

// The row stands for no node, and a walk that recurses through the tree must
// stop at it rather than try to open it. CanFetchMore is the sharp one: the
// base class answers `!children_requested_`, i.e. true, and its FetchMore
// would then Check() this row's null NodeRef and take the client down.
TEST_F(ConfigurationTreeModelTest, LoadingPlaceholderOffersNothingToExpand) {
  auto node_service_tree = std::make_unique<NiceMock<MockNodeServiceTree>>();
  std::optional<scada::base::AsyncCompletion> delayed_completion;

  const scada::NodeId root_id{scada::id::RootFolder};
  node_service_.SetFetchStatus(root_id, NodeFetchStatus::NodeOnly);
  node_service_.SetFetchHandler(root_id,
                                [&](const NodeFetchStatus&) -> Awaitable<void> {
                                  delayed_completion.emplace(executor_);
                                  co_await delayed_completion->Wait();
                                });

  InitModel(std::move(node_service_tree), root_node_);
  Drain(executor_);

  auto* root = model_->GetRoot();
  ASSERT_EQ(1, model_->GetChildCount(root));
  auto* placeholder = model_->GetChild(root, 0);
  ASSERT_TRUE(
      static_cast<ConfigurationTreeNode*>(placeholder)->IsLoadingPlaceholder());

  EXPECT_FALSE(model_->HasChildren(placeholder));
  EXPECT_FALSE(model_->CanFetchMore(placeholder));
  EXPECT_EQ(0, model_->GetChildCount(placeholder));
  // Selecting it would publish a null node to every selection-driven command.
  EXPECT_FALSE(model_->IsSelectable(placeholder, 0));
  EXPECT_EQ(scada::aui::kNoIcon, model_->GetIcon(placeholder));

  ASSERT_TRUE(delayed_completion.has_value());
  delayed_completion->Complete();
  Drain(executor_);
}

// UpdateChildTreeNodes sweeps out rows that answer to no browse target, which
// is exactly what the placeholder is — so an ordinary model-change event
// arriving mid-fetch used to be able to take the only feedback down and leave
// the pane blank again for the rest of the wait.
TEST_F(ConfigurationTreeModelTest, ModelChangeWhilePendingKeepsThePlaceholder) {
  auto node_service_tree = std::make_unique<NiceMock<MockNodeServiceTree>>();
  std::optional<scada::base::AsyncCompletion> delayed_completion;

  const scada::NodeId root_id{scada::id::RootFolder};
  node_service_.SetFetchStatus(root_id, NodeFetchStatus::NodeOnly);
  node_service_.SetFetchHandler(root_id,
                                [&](const NodeFetchStatus&) -> Awaitable<void> {
                                  delayed_completion.emplace(executor_);
                                  co_await delayed_completion->Wait();
                                });

  InitModel(std::move(node_service_tree), root_node_);
  Drain(executor_);
  ASSERT_NE(observer_, nullptr);

  auto* root = model_->GetRoot();
  ASSERT_EQ(1, model_->GetChildCount(root));

  // Still nothing browsable under the root — GetChildren answers {} by default.
  observer_->OnNodeChildrenChanged(root_id);

  ASSERT_EQ(1, model_->GetChildCount(root));
  EXPECT_TRUE(static_cast<ConfigurationTreeNode*>(model_->GetChild(root, 0))
                  ->IsLoadingPlaceholder());

  ASSERT_TRUE(delayed_completion.has_value());
  delayed_completion->Complete();
  Drain(executor_);
}

// The complement: when real rows do arrive by that route, the stand-in has
// nothing left to stand for and must go, rather than sitting under them
// claiming a fetch is still running.
TEST_F(ConfigurationTreeModelTest,
       ChildrenArrivingByModelChangeRetireThePlaceholder) {
  auto node_service_tree = std::make_unique<NiceMock<MockNodeServiceTree>>();
  std::optional<scada::base::AsyncCompletion> delayed_completion;

  const scada::NodeId root_id{scada::id::RootFolder};
  node_service_.SetFetchStatus(root_id, NodeFetchStatus::NodeOnly);
  node_service_.SetFetchHandler(root_id,
                                [&](const NodeFetchStatus&) -> Awaitable<void> {
                                  delayed_completion.emplace(executor_);
                                  co_await delayed_completion->Wait();
                                });

  EXPECT_CALL(*node_service_tree, GetChildren(_))
      .WillRepeatedly(Return(std::vector<NodeServiceTree::ChildRef>{
          {.reference_type_id = scada::id::Organizes,
           .child_node = MakeTestNodeRef(kNodeId1)}}));

  InitModel(std::move(node_service_tree), root_node_);
  Drain(executor_);
  ASSERT_NE(observer_, nullptr);

  auto* root = model_->GetRoot();
  ASSERT_EQ(1, model_->GetChildCount(root));
  ASSERT_TRUE(static_cast<ConfigurationTreeNode*>(model_->GetChild(root, 0))
                  ->IsLoadingPlaceholder());

  observer_->OnNodeChildrenChanged(root_id);

  ASSERT_EQ(1, model_->GetChildCount(root));
  auto* child = static_cast<ConfigurationTreeNode*>(model_->GetChild(root, 0));
  EXPECT_FALSE(child->IsLoadingPlaceholder());
  EXPECT_EQ(child->node().node_id(), kNodeId1);

  ASSERT_TRUE(delayed_completion.has_value());
  delayed_completion->Complete();
  Drain(executor_);
}

// The glyph table is indexed by those tile indices, inherited from the sliced
// bitmap strip it replaced. A short table would silently hand rows a null
// icon; a long one means an enum value was dropped without its glyph.
TEST(ConfigurationTreeGlyphs, TableCoversEveryImageIndex) {
  EXPECT_EQ(std::size(kItemGlyphs),
            static_cast<std::size_t>(GlyphIndices::IMAGE_COUNT));
  for (std::string_view path : kItemGlyphs) {
    EXPECT_TRUE(path.starts_with(":/icons/")) << path;
    EXPECT_TRUE(path.ends_with(".svg")) << path;
  }
}

// The favourites tree's own table, indexed by FavouritesWindowNode::GetIcon
// (0 table window, 1 graph window, 2 folder). A favourite of any other view
// type returns -1 and shows no glyph, which is why the table has no default
// entry and its size is exactly three.
TEST(ConfigurationTreeGlyphs, WindowTypeTableCoversTheFavouriteKinds) {
  ASSERT_EQ(std::size(kWindowTypeGlyphs), 3u);
  for (std::string_view path : kWindowTypeGlyphs) {
    EXPECT_TRUE(path.starts_with(":/icons/")) << path;
    EXPECT_TRUE(path.ends_with(".svg")) << path;
  }
  // A saved table window and a saved graph window must not look alike — the
  // glyph is the only thing distinguishing them in the list.
  EXPECT_NE(kWindowTypeGlyphs[0], kWindowTypeGlyphs[1]);
}

// Every mapped path must name an asset that exists and is listed in the
// resource file. The tables above only check the shape of the strings; a typo,
// or a glyph missing from res/client.qrc, yields a null icon and a silently
// blank row — and neither shows up until someone looks at the UI.
//
// Checked against the source tree rather than the compiled resource, because
// this test target does not link res/client.qrc.
TEST(ConfigurationTreeGlyphs, EveryMappedGlyphIsShipped) {
  // .../client/modules/configuration/tree/<this file>
  const std::filesystem::path client_root = std::filesystem::path{__FILE__}
                                                .parent_path()
                                                .parent_path()
                                                .parent_path()
                                                .parent_path();
  std::ifstream qrc_stream{client_root / "res" / "client.qrc"};
  ASSERT_TRUE(qrc_stream) << "res/client.qrc is missing";
  const std::string qrc{std::istreambuf_iterator<char>{qrc_stream},
                        std::istreambuf_iterator<char>{}};

  const auto check = [&](std::string_view resource_path) {
    constexpr std::string_view kPrefix = ":/";
    ASSERT_TRUE(resource_path.starts_with(kPrefix)) << resource_path;
    const std::string relative{resource_path.substr(kPrefix.size())};

    EXPECT_TRUE(std::filesystem::exists(client_root / "res" / relative))
        << relative << " is mapped but not in res/";
    EXPECT_NE(qrc.find("<file>" + relative + "</file>"), std::string::npos)
        << relative << " is mapped but not listed in res/client.qrc";
  };

  for (std::string_view path : kItemGlyphs)
    check(path);
  for (std::string_view path : kWindowTypeGlyphs)
    check(path);
}

// State moved out of the artwork and onto the status dot, so the four device
// tiles and the two subsystem tiles collapse onto one glyph each — the point
// of the conversion, and what keeps colour from being the sole carrier of
// meaning (docs/client/ux/principles.md §5).
TEST(ConfigurationTreeGlyphs, DeviceAndSubsystemStatesShareOneGlyph) {
  EXPECT_EQ(kItemGlyphs[GlyphIndices::IMAGE_DEVICE_RUNNING],
            kItemGlyphs[GlyphIndices::IMAGE_DEVICE_STOPPED]);
  EXPECT_EQ(kItemGlyphs[GlyphIndices::IMAGE_DEVICE_RUNNING],
            kItemGlyphs[GlyphIndices::IMAGE_DEVICE]);
  EXPECT_EQ(kItemGlyphs[GlyphIndices::IMAGE_DEVICE_RUNNING],
            kItemGlyphs[GlyphIndices::IMAGE_DEVICE_DISABLED]);
  EXPECT_EQ(kItemGlyphs[GlyphIndices::IMAGE_SUBSYSTEM_RUNNING],
            kItemGlyphs[GlyphIndices::IMAGE_SUBSYSTEM_STOPPED]);
  // A folder is not a device.
  EXPECT_NE(kItemGlyphs[GlyphIndices::IMAGE_FOLDER],
            kItemGlyphs[GlyphIndices::IMAGE_DEVICE]);
}

}  // namespace
