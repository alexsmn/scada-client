#include "configuration/tree/configuration_tree_drop_handler.h"

#include "base/test/awaitable_test.h"
#include "base/test/test_executor.h"
#include "common/formula_util.h"
#include "common/node_state.h"
#include "configuration/tree/configuration_tree_model.h"
#include "configuration/tree/node_service_tree_mock.h"
#include "model/data_items_node_ids.h"
#include "model/devices_node_ids.h"
#include "node_service/node_service_mock.h"
#include "node_service/test/fake_node_service.h"
#include "scada/co_result.h"
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

// Builds a node (plus its type/parent/data-type neighbours) inside |service|
// and returns a cursor to it. Each caller passes a dedicated service so the
// per-node graphs never share the same node-id map.
NodeRef MakeTestNodeInService(FakeNodeService& service,
                              const scada::NodeId& node_id,
                              TestNodeOptions options) {
  if (!options.type_definition_id.is_null())
    service.Add(scada::NodeState{.node_id = options.type_definition_id});
  if (!options.parent_id.is_null())
    service.Add(scada::NodeState{.node_id = options.parent_id});
  if (!options.data_type_id.is_null())
    service.Add(scada::NodeState{.node_id = options.data_type_id});

  // Each createable type is exposed as an OptionalPlaceholder
  // InstanceDeclaration child of the type — the standard-modelling replacement
  // for the Creates edge that CreateTree::CanCreate now reads via
  // GetCreatableChildTypes. Authored as HasComponent children so the type's
  // HierarchicalReferences query finds them.
  for (size_t i = 0; i < options.creates.size(); ++i) {
    service.Add(scada::NodeState{
        .node_id = scada::NodeId{static_cast<scada::NumericId>(90000 + i)},
        .type_definition_id = options.creates[i].node_id(),
        .parent_id = options.type_definition_id,
        .reference_type_id = scada::id::HasComponent,
        .references = {
            {scada::id::HasModellingRule, true,
             scada::NodeId{scada::id::ModellingRule_OptionalPlaceholder}}}});
  }

  scada::NodeAttributes attributes;
  attributes.browse_name = options.browse_name;
  attributes.display_name = options.display_name;
  attributes.data_type = options.data_type_id;

  scada::NodeState state =
      scada::NodeState{.node_id = node_id,
                       .node_class = scada::NodeClass::Object,
                       .type_definition_id = options.type_definition_id,
                       .attributes = attributes};
  if (!options.parent_id.is_null()) {
    state.parent_id = options.parent_id;
    state.reference_type_id = scada::id::Organizes;
  }

  return service.Add(std::move(state));
}

class ConfigurationTreeDropHandlerTest : public Test {
 public:
  // Creates a fresh backing service for each test node so their model graphs
  // stay isolated, and returns a cursor to the built node.
  NodeRef MakeTestNode(const scada::NodeId& node_id, TestNodeOptions options) {
    return MakeTestNodeInService(
        *node_services_.emplace_back(std::make_unique<FakeNodeService>()),
        node_id, std::move(options));
  }

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

  TestExecutor executor_;
  NiceMock<MockNodeService> node_service_;
  StrictMock<MockTaskManager> task_manager_;
  CreateTree create_tree_;
  std::unique_ptr<ConfigurationTreeModel> model_;
  // Backing services for the cursors handed out by MakeTestNode; kept alive
  // for the whole test since NodeRef stores a non-owning service pointer.
  std::vector<std::unique_ptr<FakeNodeService>> node_services_;
};

TEST_F(ConfigurationTreeDropHandlerTest,
       DataVariableDropOnDataGroupPostsInsertCoroutine) {
  auto target_node = MakeTargetNode(MakeTestNode(
      data_group_id_,
      {.type_definition_id = scada::data_items::id::DataGroupType}));
  auto dragging_node = MakeTestNode(
      channel_id_,
      {.type_definition_id = scada::devices::id::Iec61850DataVariableType,
       .data_type_id = scada::id::Boolean,
       .browse_name = scada::QualifiedName{"Channel"},
       .display_name = scada::LocalizedText{u"Channel"}});

  EXPECT_CALL(node_service_, GetNode(channel_id_))
      .WillOnce(Return(dragging_node));
  EXPECT_CALL(task_manager_, PostInsertTask(_))
      .WillOnce([&](const scada::NodeState& node_state)
                    -> scada::CoStatusOr<scada::NodeId> {
        EXPECT_EQ(node_state.type_definition_id,
                  scada::data_items::id::DiscreteItemType);
        EXPECT_EQ(node_state.parent_id, data_group_id_);
        EXPECT_EQ(node_state.attributes.browse_name,
                  scada::QualifiedName{"Channel"});
        EXPECT_EQ(node_state.attributes.display_name,
                  scada::LocalizedText{u"Channel"});
        EXPECT_EQ(node_state.attributes.data_type, scada::id::Boolean);
        EXPECT_THAT(node_state.properties, SizeIs(1));
        if (!node_state.properties.empty()) {
          EXPECT_EQ(node_state.properties[0].first,
                    scada::data_items::id::DataItemType_Input1);
          EXPECT_EQ(node_state.properties[0].second.as_string(),
                    MakeNodeIdFormula(channel_id_));
        }
        co_return scada::NodeId{100, 1};
      });

  DropAction action;
  auto handler = MakeHandler();
  EXPECT_EQ(handler.GetDropAction(channel_id_, target_node, action),
            scada::aui::DragDropTypes::DRAG_COPY);
  ASSERT_TRUE(action);

  EXPECT_EQ(action(), scada::aui::DragDropTypes::DRAG_COPY);
  Drain(executor_);
}

TEST_F(ConfigurationTreeDropHandlerTest,
       ControlObjectDropOnDataItemPostsUpdateCoroutine) {
  auto target_node = MakeTargetNode(MakeTestNode(
      data_item_id_,
      {.type_definition_id = scada::data_items::id::DataItemType}));
  auto dragging_node = MakeTestNode(
      channel_id_,
      {.type_definition_id = scada::devices::id::Iec61850ControlObjectType});

  EXPECT_CALL(node_service_, GetNode(channel_id_))
      .WillOnce(Return(dragging_node));
  EXPECT_CALL(task_manager_, PostUpdateTask(data_item_id_, _, _))
      .WillOnce([&](const scada::NodeId&, scada::NodeAttributes attributes,
                    scada::NodeProperties properties) -> scada::CoStatus {
        EXPECT_TRUE(attributes.empty());
        EXPECT_THAT(properties, SizeIs(1));
        if (!properties.empty()) {
          EXPECT_EQ(properties[0].first,
                    scada::data_items::id::DataItemType_Output);
          EXPECT_EQ(properties[0].second.as_string(),
                    MakeNodeIdFormula(channel_id_));
        }
        co_return scada::StatusCode::Good;
      });

  DropAction action;
  auto handler = MakeHandler();
  EXPECT_EQ(handler.GetDropAction(channel_id_, target_node, action),
            scada::aui::DragDropTypes::DRAG_LINK);
  ASSERT_TRUE(action);

  EXPECT_EQ(action(), scada::aui::DragDropTypes::DRAG_LINK);
  Drain(executor_);
}

TEST_F(ConfigurationTreeDropHandlerTest, MoveDropPostsReferenceCoroutine) {
  auto target_type = MakeTestNode(scada::data_items::id::DataGroupType, {});
  auto target_node = MakeTargetNode(MakeTestNode(
      new_parent_id_,
      {.type_definition_id = scada::data_items::id::DataGroupType}));
  auto dragging_node = MakeTestNode(
      channel_id_, {.type_definition_id = scada::data_items::id::DataItemType,
                    .parent_id = old_parent_id_,
                    .creates = {target_type}});

  EXPECT_CALL(node_service_, GetNode(channel_id_))
      .WillOnce(Return(dragging_node));

  {
    InSequence sequence;
    EXPECT_CALL(task_manager_,
                PostDeleteReference(scada::NodeId{scada::id::Organizes},
                                    old_parent_id_, channel_id_))
        .WillOnce([](const scada::NodeId&, const scada::NodeId&,
                     const scada::NodeId&) -> scada::CoStatus {
          co_return scada::StatusCode::Good;
        });
    EXPECT_CALL(task_manager_,
                PostAddReference(scada::NodeId{scada::id::Organizes},
                                 new_parent_id_, channel_id_))
        .WillOnce([](const scada::NodeId&, const scada::NodeId&,
                     const scada::NodeId&) -> scada::CoStatus {
          co_return scada::StatusCode::Good;
        });
  }

  DropAction action;
  auto handler = MakeHandler();
  EXPECT_EQ(handler.GetDropAction(channel_id_, target_node, action),
            scada::aui::DragDropTypes::DRAG_MOVE);
  ASSERT_TRUE(action);

  EXPECT_EQ(action(), scada::aui::DragDropTypes::DRAG_MOVE);
  Drain(executor_);
}

}  // namespace
