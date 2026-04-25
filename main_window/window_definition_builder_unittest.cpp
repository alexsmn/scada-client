#include "window_definition_builder.h"

#include "base/test/awaitable_test.h"
#include "base/test/test_executor.h"
#include "common/formula_util.h"
#include "resources/common_resources.h"
#include "controller/open_context.h"
#include "controller/window_info.h"
#include "node_service/node_model_mock.h"
#include "node_service/static/static_node_service.h"

#include <gmock/gmock.h>

using namespace testing;

namespace {

scada::NodeState MakeNodeState(scada::NodeId node_id,
                               scada::NodeClass node_class,
                               scada::NodeId type_definition_id,
                               scada::NodeId parent_id = {},
                               scada::NodeId reference_type_id = {}) {
  return {.node_id = std::move(node_id),
          .node_class = node_class,
          .type_definition_id = std::move(type_definition_id),
          .parent_id = std::move(parent_id),
          .reference_type_id = std::move(reference_type_id)};
}

}  // namespace

TEST(MakeWindowDefinition, OpenContext_Node) {
  const scada::NodeId kNodeId{"NodeId", 1};
  const auto node_model = std::make_shared<MockNodeModel>();

  EXPECT_CALL(*node_model, GetAttribute(scada::AttributeId::NodeId))
      .WillOnce(Return(kNodeId));

  EXPECT_CALL(*node_model, GetAttribute(scada::AttributeId::NodeClass))
      .WillOnce(Return(static_cast<scada::Int32>(scada::NodeClass::Variable)));

  EXPECT_CALL(*node_model, GetAttribute(scada::AttributeId::DisplayName))
      .WillOnce(Return(u"Имя в русской локали"));

  OpenContext open_context{node_model};

  const auto& window_info = WindowInfo{.title = u"Журнал событий"};

  auto window_definition =
      MakeWindowDefinition(&window_info, open_context).get();

  const auto kExpectedWindowDefinition =
      WindowDefinition{window_info}
          .set_title(u"Журнал событий: Имя в русской локали")
          .AddItem(
              std::move(WindowItem{"Item"}.SetString("path", "{TS.NodeId}")));

  EXPECT_EQ(window_definition, kExpectedWindowDefinition);
}

TEST(MakeWindowDefinition, AsyncExpandsGroupItemsOnExecutor) {
  const scada::NodeId group_id{1, 1};
  const scada::NodeId item_id{2, 1};

  StaticNodeService node_service;
  node_service.Add(MakeNodeState(group_id, scada::NodeClass::Object,
                                 scada::id::BaseObjectType));
  node_service.Add(MakeNodeState(item_id, scada::NodeClass::Variable,
                                 scada::id::BaseVariableType, group_id,
                                 scada::id::Organizes));

  auto executor = std::make_shared<TestExecutor>();
  const auto& window_info = WindowInfo{.title = u"Title"};

  auto window_definition = WaitAwaitable(
      executor,
      MakeWindowDefinitionAsync(MakeTestAnyExecutor(executor), &window_info,
                                node_service.GetNode(group_id),
                                /*expand_groups=*/true));

  const auto kExpectedWindowDefinition =
      WindowDefinition{window_info}
          .set_title(u"Title: ")
          .AddItem(std::move(WindowItem{"Item"}.SetString("path", "TS.2")));

  EXPECT_EQ(window_definition, kExpectedWindowDefinition);
}

TEST(MakeWindowDefinition, OpenContext_NodeIds_TimeRange) {
  const std::vector<scada::NodeId> kNodeIds{scada::NodeId{"NodeId1", 1},
                                            scada::NodeId{"NodeId2", 2},
                                            scada::NodeId{"NodeId3", 3}};

  const TimeRange kTimeRange{};
  OpenContext open_context{{}, kNodeIds, kTimeRange};

  const auto& window_info = WindowInfo{.title = u"Title"};

  auto window_definition =
      MakeWindowDefinition(&window_info, open_context).get();

  const auto kExpectedWindowDefinition =
      WindowDefinition{window_info}
          .AddItem(
              std::move(WindowItem{"Item"}.SetString("path", "{TS.NodeId1}")))
          .AddItem(
              std::move(WindowItem{"Item"}.SetString("path", "{TIT.NodeId2}")))
          .AddItem(std::move(
              WindowItem{"Item"}.SetString("path", "{MODBUS_DEVICES.NodeId3}")))
          .AddItem(std::move(WindowItem{"TimeRange"}.SetString("type", "Day")));

  EXPECT_EQ(window_definition, kExpectedWindowDefinition);
}
