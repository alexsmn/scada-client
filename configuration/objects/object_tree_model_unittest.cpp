#include "configuration/objects/object_tree_model.h"

#include "address_space/test/test_scada_node_states.h"
#include "base/blinker.h"
#include "base/test/test_executor.h"
#include "common/node_state.h"
#include "configuration/tree/node_service_tree_impl.h"
#include "model/data_items_node_ids.h"
#include "node_service/static/static_node_service.h"
#include "profile/profile.h"
#include "timed_data/timed_data_service_fake.h"

#include <gtest/gtest.h>

namespace {

class IconIdsAccessor : public ConfigurationTreeNode {
 public:
  using ConfigurationTreeNode::ConfigurationTreeNode;

  static constexpr int kFolder = IMAGE_FOLDER;
  static constexpr int kItem = IMAGE_ITEM;
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
