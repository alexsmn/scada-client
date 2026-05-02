#include "configuration/tree/configuration_tree_drop_handler.h"

#include "base/test/awaitable_test.h"
#include "base/test/test_executor.h"
#include "common/formula_util.h"
#include "configuration/tree/configuration_tree_model.h"
#include "configuration/tree/node_service_tree_mock.h"
#include "model/data_items_node_ids.h"
#include "model/devices_node_ids.h"
#include "node_service/node_model_mock.h"
#include "node_service/node_service_mock.h"
#include "services/create_tree.h"
#include "services/task_manager_mock.h"

#include <gmock/gmock.h>

using namespace testing;

namespace {

struct TestNodeOptions {
  scada::NodeId type_definition_id;
  scada::NodeId parent_id;
  scada::NodeId data_type_id;
  std::vector<NodeRef> creates;
  scada::QualifiedName browse_name{};
  scada::LocalizedText display_name{};
};

NodeRef MakeTestNode(const scada::NodeId& node_id, TestNodeOptions options) {
  auto node_model = std::make_shared<NiceMock<MockNodeModel>>();
  auto type_model = std::make_shared<NiceMock<MockNodeModel>>();
  auto parent_model = std::make_shared<NiceMock<MockNodeModel>>();
  auto data_type_model = std::make_shared<NiceMock<MockNodeModel>>();

  const NodeRef type_node{type_model};
  const NodeRef parent_node{parent_model};
  const NodeRef data_type_node{data_type_model};

  ON_CALL(*node_model, GetFetchStatus())
      .WillByDefault(Return(NodeFetchStatus::NodeAndChildren()));
  ON_CALL(*node_model, GetAttribute(scada::AttributeId::NodeId))
      .WillByDefault(Return(node_id));
  ON_CALL(*node_model, GetAttribute(scada::AttributeId::NodeClass))
      .WillByDefault(Return(static_cast<scada::Int32>(
          scada::NodeClass::Object)));
  ON_CALL(*node_model, GetAttribute(scada::AttributeId::BrowseName))
      .WillByDefault(Return(options.browse_name));
  ON_CALL(*node_model, GetAttribute(scada::AttributeId::DisplayName))
      .WillByDefault(Return(options.display_name));
  ON_CALL(*node_model, GetTarget(_, _))
      .WillByDefault([type_node, parent_node,
                      parent_id = options.parent_id](
                         const scada::NodeId& reference_type_id,
                         bool forward) -> NodeRef {
        if (forward &&
            reference_type_id == scada::id::HasTypeDefinition) {
          return type_node;
        }
        if (!forward && reference_type_id == scada::id::HierarchicalReferences &&
            !parent_id.is_null()) {
          return parent_node;
        }
        return nullptr;
      });
  ON_CALL(*node_model, GetDataType())
      .WillByDefault([data_type_node, data_type_id = options.data_type_id] {
        return data_type_id.is_null() ? NodeRef{} : data_type_node;
      });

  ON_CALL(*type_model, GetFetchStatus())
      .WillByDefault(Return(NodeFetchStatus::NodeAndChildren()));
  ON_CALL(*type_model, GetAttribute(scada::AttributeId::NodeId))
      .WillByDefault(Return(options.type_definition_id));
  ON_CALL(*type_model, GetTarget(_, _)).WillByDefault(Return(NodeRef{}));
  ON_CALL(*type_model, GetTargets(scada::id::Creates, true))
      .WillByDefault(Return(options.creates));

  ON_CALL(*parent_model, GetFetchStatus())
      .WillByDefault(Return(NodeFetchStatus::NodeAndChildren()));
  ON_CALL(*parent_model, GetAttribute(scada::AttributeId::NodeId))
      .WillByDefault(Return(options.parent_id));

  ON_CALL(*data_type_model, GetFetchStatus())
      .WillByDefault(Return(NodeFetchStatus::NodeAndChildren()));
  ON_CALL(*data_type_model, GetAttribute(scada::AttributeId::NodeId))
      .WillByDefault(Return(options.data_type_id));

  return node_model;
}

class ConfigurationTreeDropHandlerTest : public Test {
 public:
  ConfigurationTreeNode* MakeTargetNode(NodeRef node) {
    auto node_service_tree = std::make_unique<NiceMock<MockNodeServiceTree>>();

    ON_CALL(*node_service_tree, GetRoot()).WillByDefault(Return(node));
    ON_CALL(*node_service_tree, HasChildren(_)).WillByDefault(Return(false));
    ON_CALL(*node_service_tree, GetChildren(_))
        .WillByDefault(Return(std::vector<NodeServiceTree::ChildRef>{}));

    model_ = std::make_unique<ConfigurationTreeModel>(
        ConfigurationTreeModelContext{executor_, std::move(node_service_tree)});
    model_->Init();
    return static_cast<ConfigurationTreeNode*>(model_->GetRoot());
  }

  ConfigurationTreeDropHandler MakeHandler() {
    return ConfigurationTreeDropHandler{ConfigurationTreeDropHandlerContext{
        executor_, node_service_, task_manager_, create_tree_}};
  }

  const scada::NodeId data_group_id_{10, 1};
  const scada::NodeId data_item_id_{11, 1};
  const scada::NodeId channel_id_{12, 1};
  const scada::NodeId old_parent_id_{13, 1};
  const scada::NodeId new_parent_id_{14, 1};

  std::shared_ptr<TestExecutor> executor_ = std::make_shared<TestExecutor>();
  NiceMock<MockNodeService> node_service_;
  StrictMock<MockTaskManager> task_manager_;
  CreateTree create_tree_;
  std::unique_ptr<ConfigurationTreeModel> model_;
};

TEST_F(ConfigurationTreeDropHandlerTest,
       DataVariableDropOnDataGroupPostsInsertCoroutine) {
  auto target_node = MakeTargetNode(MakeTestNode(
      data_group_id_, {.type_definition_id = data_items::id::DataGroupType}));
  auto dragging_node = MakeTestNode(
      channel_id_,
      {.type_definition_id = devices::id::Iec61850DataVariableType,
       .data_type_id = scada::id::Boolean,
       .browse_name = scada::QualifiedName{"Channel"},
       .display_name = scada::LocalizedText{u"Channel"}});

  EXPECT_CALL(node_service_, GetNode(channel_id_))
      .WillOnce(Return(dragging_node));
  EXPECT_CALL(task_manager_, PostInsertTask(_))
      .WillOnce([&](const scada::NodeState& node_state) {
        EXPECT_EQ(node_state.type_definition_id,
                  data_items::id::DiscreteItemType);
        EXPECT_EQ(node_state.parent_id, data_group_id_);
        EXPECT_EQ(node_state.attributes.browse_name,
                  scada::QualifiedName{"Channel"});
        EXPECT_EQ(node_state.attributes.display_name,
                  scada::LocalizedText{u"Channel"});
        EXPECT_EQ(node_state.attributes.data_type, scada::id::Boolean);
        EXPECT_THAT(node_state.properties, SizeIs(1));
        if (!node_state.properties.empty()) {
          EXPECT_EQ(node_state.properties[0].first,
                    data_items::id::DataItemType_Input1);
          EXPECT_EQ(node_state.properties[0].second.as_string(),
                    MakeNodeIdFormula(channel_id_));
        }
        return make_resolved_promise(scada::NodeId{100, 1});
      });

  DropAction action;
  auto handler = MakeHandler();
  EXPECT_EQ(handler.GetDropAction(channel_id_, target_node, action),
            aui::DragDropTypes::DRAG_COPY);
  ASSERT_TRUE(action);

  EXPECT_EQ(action(), aui::DragDropTypes::DRAG_COPY);
  Drain(executor_);
}

TEST_F(ConfigurationTreeDropHandlerTest,
       ControlObjectDropOnDataItemPostsUpdateCoroutine) {
  auto target_node = MakeTargetNode(MakeTestNode(
      data_item_id_, {.type_definition_id = data_items::id::DataItemType}));
  auto dragging_node = MakeTestNode(
      channel_id_,
      {.type_definition_id = devices::id::Iec61850ControlObjectType});

  EXPECT_CALL(node_service_, GetNode(channel_id_))
      .WillOnce(Return(dragging_node));
  EXPECT_CALL(task_manager_, PostUpdateTask(data_item_id_, _, _))
      .WillOnce([&](const scada::NodeId&, scada::NodeAttributes attributes,
                    scada::NodeProperties properties) {
        EXPECT_TRUE(attributes.empty());
        EXPECT_THAT(properties, SizeIs(1));
        if (!properties.empty()) {
          EXPECT_EQ(properties[0].first, data_items::id::DataItemType_Output);
          EXPECT_EQ(properties[0].second.as_string(),
                    MakeNodeIdFormula(channel_id_));
        }
        return make_resolved_promise();
      });

  DropAction action;
  auto handler = MakeHandler();
  EXPECT_EQ(handler.GetDropAction(channel_id_, target_node, action),
            aui::DragDropTypes::DRAG_LINK);
  ASSERT_TRUE(action);

  EXPECT_EQ(action(), aui::DragDropTypes::DRAG_LINK);
  Drain(executor_);
}

TEST_F(ConfigurationTreeDropHandlerTest, MoveDropPostsReferenceCoroutine) {
  auto target_type = MakeTestNode(data_items::id::DataGroupType, {});
  auto target_node = MakeTargetNode(
      MakeTestNode(new_parent_id_,
                   {.type_definition_id = data_items::id::DataGroupType}));
  auto dragging_node = MakeTestNode(
      channel_id_, {.type_definition_id = data_items::id::DataItemType,
                    .parent_id = old_parent_id_,
                    .creates = {target_type}});

  EXPECT_CALL(node_service_, GetNode(channel_id_))
      .WillOnce(Return(dragging_node));

  {
    InSequence sequence;
    EXPECT_CALL(task_manager_,
                PostDeleteReference(scada::NodeId{scada::id::Organizes},
                                    old_parent_id_, channel_id_))
        .WillOnce(Return(make_resolved_promise()));
    EXPECT_CALL(task_manager_,
                PostAddReference(scada::NodeId{scada::id::Organizes},
                                 new_parent_id_, channel_id_))
        .WillOnce(Return(make_resolved_promise()));
  }

  DropAction action;
  auto handler = MakeHandler();
  EXPECT_EQ(handler.GetDropAction(channel_id_, target_node, action),
            aui::DragDropTypes::DRAG_MOVE);
  ASSERT_TRUE(action);

  EXPECT_EQ(action(), aui::DragDropTypes::DRAG_MOVE);
  Drain(executor_);
}

}  // namespace
