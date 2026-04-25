#include "configuration/objects/object_tree_model.h"

#include "address_space/test/test_scada_node_states.h"
#include "base/blinker.h"
#include "base/test/test_executor.h"
#include "common/node_state.h"
#include "configuration/tree/node_service_tree_mock.h"
#include "configuration/tree/node_service_tree_impl.h"
#include "model/data_items_node_ids.h"
#include "node_service/node_model_mock.h"
#include "node_service/node_service_mock.h"
#include "node_service/static/static_node_service.h"
#include "profile/profile.h"
#include "timed_data/timed_data_service_mock.h"
#include "timed_data/timed_data_service_fake.h"

#include <gmock/gmock.h>

using namespace testing;

namespace {

class IconIdsAccessor : public ConfigurationTreeNode {
 public:
  using ConfigurationTreeNode::ConfigurationTreeNode;

  static constexpr int kFolder = IMAGE_FOLDER;
  static constexpr int kItem = IMAGE_ITEM;
};

NodeRef MakeObjectTreeNodeModel(const scada::NodeId& node_id,
                                NodeFetchStatus fetch_status) {
  auto node_model = std::make_shared<NiceMock<MockNodeModel>>();
  auto type_model = std::make_shared<NiceMock<MockNodeModel>>();

  const NodeRef type_node{type_model};

  ON_CALL(*node_model, GetFetchStatus()).WillByDefault(Return(fetch_status));
  ON_CALL(*node_model, GetAttribute(scada::AttributeId::NodeId))
      .WillByDefault(Return(node_id));
  ON_CALL(*node_model, GetAttribute(scada::AttributeId::NodeClass))
      .WillByDefault(Return(static_cast<scada::Int32>(
          scada::NodeClass::Variable)));
  ON_CALL(*node_model,
          GetTarget(scada::NodeId{scada::id::HasTypeDefinition}, true))
      .WillByDefault(Return(type_node));

  ON_CALL(*type_model, GetAttribute(scada::AttributeId::NodeId))
      .WillByDefault(Return(data_items::id::DataItemType));
  ON_CALL(*type_model, GetTarget(scada::NodeId{scada::id::HasSubtype}, false))
      .WillByDefault(Return(NodeRef{}));

  return node_model;
}

class CountingTreeModelObserver : public aui::TreeModelObserver {
 public:
  void OnTreeNodeChanged(void* node) override { changed_nodes.push_back(node); }

  std::vector<void*> changed_nodes;
};

class StaticVisibleNode : public VisibleNode {
 public:
  explicit StaticVisibleNode(std::u16string text) : text_{std::move(text)} {}

  std::u16string GetText() const override { return text_; }

 private:
  std::u16string text_;
};

class TestObjectTreeModel : public ObjectTreeModel {
 public:
  using ObjectTreeModel::ObjectTreeModel;

  int fetched_visible_node_count() const { return fetched_visible_node_count_; }

 protected:
  std::shared_ptr<VisibleNode> CreateFetchedVisibleNode(
      const NodeRef& node) override {
    ++fetched_visible_node_count_;
    return std::make_shared<StaticVisibleNode>(u"Fetched");
  }

 private:
  int fetched_visible_node_count_ = 0;
};

}  // namespace

class ObjectTreeModelTest : public ::testing::Test {
 protected:
  ObjectTreeModelTest()
      : node_service_tree_factory_{
            [](NodeServiceTreeImplContext&& context) {
              return std::make_unique<NodeServiceTreeImpl>(std::move(context));
            }},
        blinker_manager_{executor_} {}

  void SetUp() override {
    node_service_.AddAll(GetScadaNodeStates());

    node_service_.Add(
        scada::NodeState{}
            .set_node_id(kDataGroupId)
            .set_node_class(scada::NodeClass::Object)
            .set_type_definition_id(data_items::id::DataGroupType)
            .set_parent(scada::id::Organizes, data_items::id::DataItems));

    // Regression setup: object view may receive a data item with object
    // node-class semantics while it still derives from `DataItemType`.
    node_service_.Add(
        scada::NodeState{}
            .set_node_id(kDataItemId)
            .set_node_class(scada::NodeClass::Object)
            .set_type_definition_id(data_items::id::DiscreteItemType)
            .set_parent(scada::id::Organizes, data_items::id::DataItems));

    model_ = std::make_unique<ObjectTreeModel>(ObjectTreeModelContext{
        executor_,
        node_service_,
        node_service_.GetNode(data_items::id::DataItems),
        timed_data_service_,
        profile_,
        blinker_manager_,
        node_service_tree_factory_,
    });
    model_->Init();
  }

  static inline const scada::NodeId kDataGroupId{1001, 1};
  static inline const scada::NodeId kDataItemId{1002, 1};

  std::shared_ptr<Executor> executor_ = std::make_shared<TestExecutor>();
  StaticNodeService node_service_;
  FakeTimedDataService timed_data_service_;
  Profile profile_;
  NodeServiceTreeFactory node_service_tree_factory_;
  BlinkerManagerImpl blinker_manager_;
  std::unique_ptr<ObjectTreeModel> model_;
};

TEST_F(ObjectTreeModelTest, DataItemsUseItemIconEvenWhenNodeClassIsObject) {
  auto* data_group_node = model_->FindFirstTreeNode(kDataGroupId);
  auto* data_item_node = model_->FindFirstTreeNode(kDataItemId);

  ASSERT_NE(data_group_node, nullptr);
  ASSERT_NE(data_item_node, nullptr);

  EXPECT_EQ(data_group_node->GetIcon(), IconIdsAccessor::kFolder);
  EXPECT_EQ(data_item_node->GetIcon(), IconIdsAccessor::kItem);
}

class ObjectTreeModelAsyncVisibleNodeTest : public ::testing::Test {
 protected:
  void InitModel(bool remove_child_on_second_get_children = false) {
    root_node_ =
        MakeObjectTreeNodeModel(scada::id::RootFolder,
                                NodeFetchStatus::NodeAndChildren());
    child_node_ = MakeObjectTreeNodeModel(kDataItemId, NodeFetchStatus::None());

    auto node_service_tree = std::make_unique<NiceMock<MockNodeServiceTree>>();
    node_service_tree_ = node_service_tree.get();

    EXPECT_CALL(*node_service_tree_, SetObserver(_))
        .WillOnce([this](NodeServiceTree::Observer* observer) {
          node_service_tree_observer_ = observer;
        });
    EXPECT_CALL(*node_service_tree_, GetRoot()).WillOnce(Return(root_node_));

    auto child_refs = std::vector<NodeServiceTree::ChildRef>{
        {.reference_type_id = scada::id::Organizes,
         .child_node = child_node_}};
    if (remove_child_on_second_get_children) {
      EXPECT_CALL(*node_service_tree_, GetChildren(_))
          .WillOnce(Return(child_refs))
          .WillOnce(Return(std::vector<NodeServiceTree::ChildRef>{}));
    } else {
      EXPECT_CALL(*node_service_tree_, GetChildren(_))
          .WillOnce(Return(child_refs));
    }

    auto node_service_tree_holder =
        std::make_shared<std::unique_ptr<NodeServiceTree>>(
            std::move(node_service_tree));
    node_service_tree_factory_ =
        [node_service_tree_holder](NodeServiceTreeImplContext&&) mutable {
          return std::move(*node_service_tree_holder);
        };

    model_ = std::make_unique<TestObjectTreeModel>(ObjectTreeModelContext{
        executor_,
        node_service_,
        root_node_,
        timed_data_service_,
        profile_,
        blinker_manager_,
        node_service_tree_factory_,
    });
    model_->AddObserver(observer_);
    model_->Init();

    child_tree_node_ = model_->GetChild(model_->GetRoot(), 0);
    ASSERT_NE(child_tree_node_, nullptr);
  }

  void ExpectDelayedFetch() {
    auto child_model =
        std::static_pointer_cast<const MockNodeModel>(child_node_.model());
    EXPECT_CALL(*child_model, Fetch(NodeFetchStatus::NodeOnly(), _))
        .WillOnce([this](const NodeFetchStatus&,
                         const NodeModel::FetchCallback& callback) {
          delayed_fetch_callback_ = callback;
        });
  }

  void PollExecutor() {
    std::static_pointer_cast<TestExecutor>(executor_)->Poll();
  }

  void CompleteFetch() {
    auto child_model =
        std::static_pointer_cast<const MockNodeModel>(child_node_.model());
    ON_CALL(*child_model, GetFetchStatus())
        .WillByDefault(Return(NodeFetchStatus::NodeOnly()));
    delayed_fetch_callback_();
    PollExecutor();
  }

  static inline const scada::NodeId kDataItemId{2001, 1};

  std::shared_ptr<Executor> executor_ = std::make_shared<TestExecutor>();
  NiceMock<MockNodeService> node_service_;
  NiceMock<MockTimedDataService> timed_data_service_;
  Profile profile_;
  BlinkerManagerImpl blinker_manager_{executor_};
  NodeServiceTreeFactory node_service_tree_factory_;
  MockNodeServiceTree* node_service_tree_ = nullptr;
  NodeServiceTree::Observer* node_service_tree_observer_ = nullptr;
  CountingTreeModelObserver observer_;
  NodeRef root_node_;
  NodeRef child_node_;
  void* child_tree_node_ = nullptr;
  NodeModel::FetchCallback delayed_fetch_callback_;
  std::unique_ptr<TestObjectTreeModel> model_;
};

TEST_F(ObjectTreeModelAsyncVisibleNodeTest,
       DelayedVisibleNodeFetchUpdatesProxyWhenRowStillVisible) {
  InitModel();
  ExpectDelayedFetch();

  model_->SetNodeVisible(child_tree_node_, true);
  PollExecutor();
  ASSERT_TRUE(static_cast<bool>(delayed_fetch_callback_));
  EXPECT_EQ(model_->fetched_visible_node_count(), 0);
  EXPECT_TRUE(model_->GetText(child_tree_node_, 1).empty());

  CompleteFetch();
  EXPECT_EQ(model_->fetched_visible_node_count(), 1);
  EXPECT_EQ(model_->GetText(child_tree_node_, 1), u"Fetched");
}

TEST_F(ObjectTreeModelAsyncVisibleNodeTest,
       DelayedVisibleNodeFetchAfterRowHiddenDoesNotUpdateProxy) {
  InitModel();
  ExpectDelayedFetch();

  model_->SetNodeVisible(child_tree_node_, true);
  PollExecutor();
  ASSERT_TRUE(static_cast<bool>(delayed_fetch_callback_));

  model_->SetNodeVisible(child_tree_node_, false);
  CompleteFetch();
  EXPECT_EQ(model_->fetched_visible_node_count(), 0);
  EXPECT_TRUE(model_->GetText(child_tree_node_, 1).empty());
}

TEST_F(ObjectTreeModelAsyncVisibleNodeTest,
       DelayedVisibleNodeFetchAfterTreeNodeRemovalDoesNotUpdateProxy) {
  InitModel(/*remove_child_on_second_get_children=*/true);
  ExpectDelayedFetch();

  model_->SetNodeVisible(child_tree_node_, true);
  PollExecutor();
  ASSERT_TRUE(static_cast<bool>(delayed_fetch_callback_));

  ASSERT_NE(node_service_tree_observer_, nullptr);
  node_service_tree_observer_->OnNodeChildrenChanged(scada::id::RootFolder);
  ASSERT_EQ(model_->GetChildCount(model_->GetRoot()), 0);

  CompleteFetch();
  EXPECT_EQ(model_->fetched_visible_node_count(), 0);
}
