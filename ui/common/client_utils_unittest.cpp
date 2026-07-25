#include "ui/common/client_utils.h"

#include "base/any_executor.h"
#include "base/test/awaitable_test.h"
#include "base/test/test_executor.h"
#include "common/node_state.h"
#include "node_service/test/fake_node_service.h"

#include <gmock/gmock.h>

using namespace testing;
namespace {

scada::NodeState MakeNodeState(const scada::NodeId& node_id,
                               scada::NodeClass node_class) {
  return scada::NodeState{}.set_node_id(node_id).set_node_class(node_class);
}

}  // namespace

TEST(ClientUtilsTest, ExpandGroupItemIdsAsyncRespectsMaxCount) {
  const scada::NodeId root_id{7000, 1};
  const scada::NodeId first_id{7001, 1};
  const scada::NodeId group_id{7002, 1};
  const scada::NodeId second_id{7003, 1};
  const scada::NodeId third_id{7004, 1};

  // root --Organizes--> first
  //      --HasComponent--> group --Organizes--> {second, third}
  FakeNodeService node_service;
  const NodeRef root =
      node_service.Add(MakeNodeState(root_id, scada::NodeClass::Object));
  node_service.Add(MakeNodeState(first_id, scada::NodeClass::Variable)
                       .set_parent(scada::id::Organizes, root_id));
  node_service.Add(MakeNodeState(group_id, scada::NodeClass::Object)
                       .set_parent(scada::id::HasComponent, root_id));
  node_service.Add(MakeNodeState(second_id, scada::NodeClass::Variable)
                       .set_parent(scada::id::Organizes, group_id));
  node_service.Add(MakeNodeState(third_id, scada::NodeClass::Variable)
                       .set_parent(scada::id::Organizes, group_id));

  TestExecutor executor;
  auto node_ids =
      WaitAwaitable(executor, ExpandGroupItemIdsAsync(executor, root,
                                                      /*max_count=*/2));

  EXPECT_THAT(node_ids, UnorderedElementsAre(first_id, second_id));
}

TEST(ClientUtilsTest, ExpandGroupItemIdsAsyncZeroLimitDoesNotFetch) {
  const scada::NodeId root_id{7100, 1};
  FakeNodeService node_service;
  const NodeRef root =
      node_service.Add(MakeNodeState(root_id, scada::NodeClass::Object));
  node_service.SetFetchStatus(root_id, NodeFetchStatus::None);

  TestExecutor executor;
  auto node_ids =
      WaitAwaitable(executor, ExpandGroupItemIdsAsync(executor, root,
                                                      /*max_count=*/0));

  EXPECT_TRUE(node_ids.empty());
  // A zero limit must not touch the node at all.
  EXPECT_TRUE(node_service.fetch_requests(root_id).empty());
}
