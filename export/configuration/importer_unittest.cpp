#include "importer.h"

#include "diff_data.h"
#include "model/data_items_node_ids.h"
#include "services/task_manager_mock.h"

#include <gmock/gmock.h>

using namespace testing;

// Pass child node before parent node. Ensure nodes are created correctly.
TEST(Importer, UnorderedCreatedNodes) {
  const auto& root_node_id = data_items::id::DataItems;
  const auto& data_item_id = scada::NodeId{1, NamespaceIndexes::TS};
  const auto& data_group_id = scada::NodeId{1, NamespaceIndexes::GROUP};

  MockTaskManager task_manager;

  InSequence s;

  EXPECT_CALL(task_manager,
              PostInsertTask(Field(&scada::NodeState::node_id, data_group_id)))
      .WillOnce(Return(make_resolved_promise(data_group_id)));

  EXPECT_CALL(task_manager,
              PostInsertTask(Field(&scada::NodeState::node_id, data_item_id)))
      .WillOnce(Return(make_resolved_promise(data_item_id)));

  ApplyDiffData(
      {.create_nodes = {{.node_id = data_item_id,
                         .type_definition_id = data_items::id::AnalogItemType,
                         .parent_id = data_group_id},
                        {.node_id = data_group_id,
                         .type_definition_id = data_items::id::DataGroupType,
                         .parent_id = root_node_id}}},
      task_manager);
}