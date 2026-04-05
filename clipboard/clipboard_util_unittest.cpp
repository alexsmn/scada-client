#include "clipboard_util.h"

#include "model/data_items_node_ids.h"
#include "model/namespaces.h"
#include "services/task_manager_mock.h"
#include "services/test/test_task_manager.h"

#include <gmock/gmock.h>

#include "base/debug_util.h"

using namespace testing;

namespace {

void CompareRecursive(const scada::NodeState& a, const scada::NodeState& b) {
  EXPECT_EQ(a.type_definition_id, b.type_definition_id);
  EXPECT_EQ(a.attributes, b.attributes);
  EXPECT_EQ(a.properties, b.properties);
  EXPECT_EQ(a.references, b.references);

  ASSERT_EQ(a.children.size(), b.children.size());

  for (size_t i = 0; i < a.children.size(); ++i) {
    CompareRecursive(a.children[i], b.children[i]);
  }
}

}  // namespace

TEST(PasteNodesFromNodeStateRecursive, Test) {
  TestStorage storage{data_items::id::DataItems};
  TestTaskManager task_manager{storage};

  // Have to specify parent IDs for children to be able to compare them.
  const auto& top_node = scada::NodeState{
      .node_id = {1, NamespaceIndexes::GROUP},
      .type_definition_id = data_items::id::DataGroupType,
      .parent_id = data_items::id::DataItems,
      .reference_type_id = scada::id::Organizes,
      .attributes = {.browse_name = "Group1", .display_name = u"Group1"},
      .children = {
          {.node_id = {2, NamespaceIndexes::GROUP},
           .type_definition_id = data_items::id::DataGroupType,
           .parent_id = {1, NamespaceIndexes::GROUP},
           .reference_type_id = scada::id::Organizes,
           .attributes = {.browse_name = "Group2", .display_name = u"Group2"},
           .children = {{.node_id = {1, NamespaceIndexes::TS},
                         .type_definition_id = data_items::id::DiscreteItemType,
                         .parent_id = {2, NamespaceIndexes::GROUP},
                         .reference_type_id = scada::id::Organizes,
                         .attributes = {.browse_name = "DataItem1",
                                        .display_name = u"DataItem1"}}}}}};

  PasteNodesFromNodeStateRecursive(task_manager, scada::NodeState{top_node})
      .get();

  auto* storage_root_node = storage.FindNode(data_items::id::DataItems);
  ASSERT_THAT(storage_root_node, NotNull());
  ASSERT_THAT(storage_root_node->children, SizeIs(1));

  CompareRecursive(top_node, storage_root_node->children[0]);
}
