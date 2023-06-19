#include "components/configuration_tree/configuration_tree_model.h"

#include "components/configuration_tree/node_service_tree_mock.h"
#include "core/standard_node_ids.h"
#include "node_service/node_model_mock.h"

#include <gmock/gmock.h>

using namespace testing;

namespace {

NodeRef MakeTestNodeRef(const scada::NodeId& node_id) {
  auto node_model = std::make_shared<NiceMock<MockNodeModel>>();
  ON_CALL(*node_model, GetAttribute(scada::AttributeId::NodeId))
      .WillByDefault(Return(node_id));
  return node_model;
}

}  // namespace

class ConfigurationTreeModelTest : public Test {
 public:
  void InitModel(std::unique_ptr<MockNodeServiceTree> node_service_tree);

  const NodeRef root_node_ = MakeTestNodeRef(scada::id::RootFolder);

  MockNodeServiceTree* node_service_tree_ = nullptr;
  std::unique_ptr<ConfigurationTreeModel> model_;

  inline static const scada::NodeId kNodeId1{1, 1};
  inline static const scada::NodeId kNodeId2{2, 1};
  inline static const scada::NodeId kNodeId3{3, 1};
};

void ConfigurationTreeModelTest::InitModel(
    std::unique_ptr<MockNodeServiceTree> node_service_tree) {
  node_service_tree_ = node_service_tree.get();

  EXPECT_CALL(*node_service_tree_, SetObserver(_));

  EXPECT_CALL(*node_service_tree_, GetRoot()).WillOnce(Return(root_node_));

  ON_CALL(*node_service_tree_, GetChildren(_))
      .WillByDefault(Return(std::vector<NodeServiceTree::ChildRef>{}));

  model_ = std::make_unique<ConfigurationTreeModel>(
      ConfigurationTreeModelContext{std::move(node_service_tree)});
  model_->Init();

  EXPECT_TRUE(model_->root_node() == root_node_);
}

TEST_F(ConfigurationTreeModelTest, PrefetchedChildrenAreAvailable) {
  auto node_service_tree = std::make_unique<NiceMock<MockNodeServiceTree>>();

  // Expect one call for the root node and a call per child node.
  // WARNING: Cannot use the equality matcher for the `NodeRef` parameter as it
  // seems to bring a deadlock.
  EXPECT_CALL(*node_service_tree, GetChildren(_))
      .Times(4)
      .WillOnce(Return(std::vector<NodeServiceTree::ChildRef>{
          {.reference_type_id = scada::id::Organizes,
           .child_node = MakeTestNodeRef(kNodeId1)},
          {.reference_type_id = scada::id::Organizes,
           .child_node = MakeTestNodeRef(kNodeId2)},
          {.reference_type_id = scada::id::Organizes,
           .child_node = MakeTestNodeRef(kNodeId3)}}))
      .WillRepeatedly(DoDefault());

  InitModel(std::move(node_service_tree));

  auto* root = model_->GetRoot();
  EXPECT_EQ(3, model_->GetChildCount(root));
}
