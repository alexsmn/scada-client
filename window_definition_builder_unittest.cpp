#include "window_definition_builder.h"

#include "common/formula_util.h"
#include "common_resources.h"
#include "controller.h"
#include "node_service/node_model_mock.h"

#include <gmock/gmock.h>

using namespace testing;

TEST(MakeWindowDefinition, OpenContext_Node) {
  const scada::NodeId kNodeId{"NodeId", 1};
  const auto node_model = std::make_shared<MockNodeModel>();
  EXPECT_CALL(*node_model, GetAttribute(scada::AttributeId::NodeId))
      .WillOnce(Return(kNodeId));
  EXPECT_CALL(*node_model, GetAttribute(scada::AttributeId::NodeClass))
      .WillOnce(Return(static_cast<scada::Int32>(scada::NodeClass::Variable)));
  EXPECT_CALL(*node_model, GetAttribute(scada::AttributeId::DisplayName))
      .WillOnce(Return(L"Имя в русской локали"));
  OpenContext open_context{node_model};
  const unsigned kWindowType = ID_EVENT_JOURNAL_VIEW;
  auto window_definition =
      MakeWindowDefinition(open_context, kWindowType).get();
  const auto kExpectedWindowDefinition =
      WindowDefinition{GetWindowInfo(kWindowType)}
          .set_title(L"Журнал событий: Имя в русской локали")
          .AddItem(
              std::move(WindowItem{"Item"}.SetString("path", "{TS.NodeId}")));
  EXPECT_EQ(window_definition, kExpectedWindowDefinition);
}

TEST(MakeWindowDefinition, OpenContext_NodeIds_TimeRange) {
  const std::vector<scada::NodeId> kNodeIds{scada::NodeId{"NodeId1", 1},
                                            scada::NodeId{"NodeId2", 2},
                                            scada::NodeId{"NodeId3", 3}};
  const TimeRange kTimeRange{};
  OpenContext open_context{{}, kNodeIds, kTimeRange};
  const unsigned kWindowType = ID_EVENT_JOURNAL_VIEW;
  auto window_definition =
      MakeWindowDefinition(open_context, kWindowType).get();
  const auto kExpectedWindowDefinition =
      WindowDefinition{GetWindowInfo(kWindowType)}
          .AddItem(
              std::move(WindowItem{"Item"}.SetString("path", "{TS.NodeId1}")))
          .AddItem(
              std::move(WindowItem{"Item"}.SetString("path", "{TIT.NodeId2}")))
          .AddItem(std::move(
              WindowItem{"Item"}.SetString("path", "{MODBUS_DEVICES.NodeId3}")))
          .AddItem(std::move(WindowItem{"TimeRange"}.SetString("type", "Day")));
  EXPECT_EQ(window_definition, kExpectedWindowDefinition);
}
