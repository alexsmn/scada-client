#include "node_serialization.h"

#include "common/node_state.h"
#include "common/node_state_util.h"
#include "common/type_system_mock.h"
#include "model/data_items_node_ids.h"
#include "node_service/test/fake_node_service.h"

#include <gmock/gmock.h>

using namespace testing;

TEST(NodeSerialization, DISABLED_NodeToData) {
  NiceMock<MockTypeSystem> type_system;

  const scada::NodeState source_node_state{
      scada::NodeId{1, 1},
      scada::NodeClass::Variable,
      scada::data_items::id::DataItemType,
      scada::data_items::id::DataItems,
      scada::id::Organizes,
      scada::NodeAttributes{}.set_display_name(u"Display Name"),
      scada::NodeProperties{
          {scada::data_items::id::DataItemType_Alias, "Alias"}},
      {},
      {},
      {}};

  FakeNodeService node_service;
  NodeRef node = node_service.Add(source_node_state);

  scada::NodeState node_state;
  NodeToData(node, node_state, true, true);

  EXPECT_EQ(node_state.attributes.display_name,
            source_node_state.attributes.display_name);
  EXPECT_EQ(node_state.properties, source_node_state.properties);
  EXPECT_EQ(node_state.references, source_node_state.references);
}
