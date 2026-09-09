#include "ui/common/client_utils.h"

#include "base/any_executor.h"
#include "base/test/awaitable_test.h"
#include "base/test/test_executor.h"
#include "common/node_state.h"
#include "node_service/test/fake_node_service.h"

#include <gmock/gmock.h>

using namespace testing;

TEST(ClientUtilsTest, ExpandGroupItemIdsAsyncRespectsMaxCount) {
  const scada::NodeId root_id{7000, 1};
  const scada::NodeId first_id{7001, 1};
  const scada::NodeId group_id{7002, 1};
  const scada::NodeId second_id{7003, 1};
  const scada::NodeId third_id{7004, 1};

  // root --Organizes--> first
  //      --HasComponent--> group --Organizes--> {second, third}
  FakeNodeService node_service;
  const NodeRef root = node_service.Add(
      {.node_id = root_id, .node_class = scada::NodeClass::Object});
  node_service.Add({.node_id = first_id,
                    .node_class = scada::NodeClass::Variable,
                    .parent_id = root_id,
                    .reference_type_id = scada::id::Organizes});
  node_service.Add({.node_id = group_id,
                    .node_class = scada::NodeClass::Object,
                    .parent_id = root_id,
                    .reference_type_id = scada::id::HasComponent});
  node_service.Add({.node_id = second_id,
                    .node_class = scada::NodeClass::Variable,
                    .parent_id = group_id,
                    .reference_type_id = scada::id::Organizes});
  node_service.Add({.node_id = third_id,
                    .node_class = scada::NodeClass::Variable,
                    .parent_id = group_id,
                    .reference_type_id = scada::id::Organizes});

  TestExecutor executor;
  auto node_ids =
      WaitAwaitable(executor, ExpandGroupItemIdsAsync(executor, root,
                                                      /*max_count=*/2));

  EXPECT_THAT(node_ids, UnorderedElementsAre(first_id, second_id));
}

TEST(ClientUtilsTest, ExpandGroupItemIdsAsyncZeroLimitDoesNotFetch) {
  const scada::NodeId root_id{7100, 1};
  FakeNodeService node_service;
  const NodeRef root = node_service.Add(
      {.node_id = root_id, .node_class = scada::NodeClass::Object});
  node_service.SetFetchStatus(root_id, NodeFetchStatus::None);

  TestExecutor executor;
  auto node_ids =
      WaitAwaitable(executor, ExpandGroupItemIdsAsync(executor, root,
                                                      /*max_count=*/0));

  EXPECT_TRUE(node_ids.empty());
  // A zero limit must not touch the node at all.
  EXPECT_TRUE(node_service.fetch_requests(root_id).empty());
}

// The named-node lists behind the device pickers sort for display: case-blind
// and alphabet-aware, not by code point (task 716).
TEST(ClientUtilsTest, SortNamedNodesOrdersForDisplay) {
  NamedNodes nodes;
  for (const char16_t* name :
       {u"Яблоко", u"beta", u"Alpha", u"ёлка", u"Ель", u"gamma"}) {
    nodes.emplace_back(name, NodeRef{});
  }

  SortNamedNodes(nodes);

  std::vector<std::u16string> names;
  for (const auto& [name, node] : nodes)
    names.push_back(name);
  EXPECT_THAT(names, ElementsAre(u"Alpha", u"beta", u"gamma", u"Ель", u"ёлка",
                                 u"Яблоко"));
}
