#include "node_serialization.h"

#include "common/node_state.h"
#include "common/node_state_util.h"
#include "common/type_system_mock.h"
#include "model/data_items_node_ids.h"
#include "node_service/node_model_mock.h"
#include "node_service/test/model_node_service.h"

#include <gmock/gmock.h>

using namespace testing;

TEST(NodeSerialization, DISABLED_NodeToData) {
  NiceMock<MockTypeSystem> type_system;

  ModelNodeService node_service;
  auto node_model = std::make_shared<NiceMock<MockNodeModel>>();
  NodeRef node = node_service.Add(scada::NodeId{1, 1}, node_model);

  scada::NodeState source_node_state{
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
  ON_CALL(*node_model, GetAttribute(_))
      .WillByDefault(Invoke([&](scada::AttributeId attribute_id) {
        return source_node_state.GetAttribute(attribute_id)
            .value_or(scada::Variant{});
      }));
  /*ON_CALL(*node_model, GetReferences(_, _))
      .WillByDefault(
          Invoke([&](const scada::NodeId& reference_type_id, bool forward) {
            auto references = scada::Browse(type_system, source_node_state,
                                            reference_type_id, forward);
            return Map(references, [&](const scada::ReferenceDescription& ref) {
              return NodeRef::Reference{GetNodeRef(ref.reference_type_id),
                                        GetNodeRef(ref.node_id), ref.forward};
            });
          }));*/

  scada::NodeState node_state;
  NodeToData(node, node_state, true, true);

  EXPECT_EQ(node_state.attributes.display_name,
            source_node_state.attributes.display_name);
  EXPECT_EQ(node_state.properties, source_node_state.properties);
  EXPECT_EQ(node_state.references, source_node_state.references);
}
