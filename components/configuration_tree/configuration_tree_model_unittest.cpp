#include "components/configuration_tree/configuration_tree_model.h"

#include "components/configuration_tree/node_service_tree_mock.h"
#include "core/standard_node_ids.h"
#include "node_service/node_model_mock.h"

#include <gmock/gmock.h>

using namespace testing;

class ConfigurationTreeModelTest : public Test {
 public:
  virtual void SetUp() override;

  MockNodeServiceTree* node_service_tree_ = nullptr;
  std::shared_ptr<MockNodeModel> root_node_model_;
  std::unique_ptr<ConfigurationTreeModel> model_;
};

void ConfigurationTreeModelTest::SetUp() {
  auto node_service_tree = std::make_unique<MockNodeServiceTree>();
  node_service_tree_ = node_service_tree.get();

  root_node_model_ = std::make_shared<MockNodeModel>();

  EXPECT_CALL(*node_service_tree_, SetObserver(_));
  EXPECT_CALL(*node_service_tree_, GetRoot())
      .WillOnce(Return(NodeRef{root_node_model_}));
  EXPECT_CALL(*root_node_model_, GetAttribute(scada::AttributeId::NodeId))
      .WillRepeatedly(Return(scada::id::RootFolder));
  EXPECT_CALL(*node_service_tree_, GetChildren(_)).Times(0);

  model_ = std::make_unique<ConfigurationTreeModel>(
      ConfigurationTreeModelContext{std::move(node_service_tree)});

  EXPECT_TRUE(model_->root_node() == NodeRef{root_node_model_});
}

TEST_F(ConfigurationTreeModelTest, Test) {
  /*auto* root = model_->GetRoot();
  EXPECT_EQ(3, model_->GetChildCount(root));*/
}
