#include "ui/common/client_utils.h"

#include "base/any_executor.h"
#include "base/test/awaitable_test.h"
#include "base/test/test_executor.h"
#include "node_service/node_model_mock.h"
#include "node_service/test/model_node_service.h"

#include <gmock/gmock.h>

using namespace testing;
namespace {

std::shared_ptr<NiceMock<MockNodeModel>> MakeNodeModel(
    const scada::NodeId& node_id,
    scada::NodeClass node_class,
    NodeFetchStatus fetch_status = NodeFetchStatus::NodeAndChildren) {
  auto node_model = std::make_shared<NiceMock<MockNodeModel>>();
  ON_CALL(*node_model, GetFetchStatus()).WillByDefault(Return(fetch_status));
  ON_CALL(*node_model, GetStatus())
      .WillByDefault(Return(scada::StatusCode::Good));
  ON_CALL(*node_model, GetAttribute(scada::AttributeId::NodeId))
      .WillByDefault(Return(node_id));
  ON_CALL(*node_model, GetAttribute(scada::AttributeId::NodeClass))
      .WillByDefault(Return(static_cast<scada::Int32>(node_class)));
  ON_CALL(*node_model, Fetch(_))
      .WillByDefault(
          [](const NodeFetchStatus&) -> Awaitable<void> { co_return; });
  return node_model;
}

}  // namespace

TEST(ClientUtilsTest, ExpandGroupItemIdsAsyncRespectsMaxCount) {
  const scada::NodeId root_id{7000, 1};
  const scada::NodeId first_id{7001, 1};
  const scada::NodeId group_id{7002, 1};
  const scada::NodeId second_id{7003, 1};
  const scada::NodeId third_id{7004, 1};

  ModelNodeService node_service;
  auto root_model = MakeNodeModel(root_id, scada::NodeClass::Object);
  auto first_model = MakeNodeModel(first_id, scada::NodeClass::Variable);
  auto group_model = MakeNodeModel(group_id, scada::NodeClass::Object);
  auto second_model = MakeNodeModel(second_id, scada::NodeClass::Variable);
  auto third_model = MakeNodeModel(third_id, scada::NodeClass::Variable);

  const NodeRef root = node_service.Add(root_id, root_model);
  const NodeRef first = node_service.Add(first_id, first_model);
  const NodeRef group = node_service.Add(group_id, group_model);
  const NodeRef second = node_service.Add(second_id, second_model);
  const NodeRef third = node_service.Add(third_id, third_model);

  ON_CALL(*root_model, GetTargets(scada::NodeId{scada::id::Organizes}, true))
      .WillByDefault(Return(std::vector<NodeRef>{first}));
  ON_CALL(*root_model, GetTargets(scada::NodeId{scada::id::HasComponent}, true))
      .WillByDefault(Return(std::vector<NodeRef>{group}));
  ON_CALL(*group_model, GetTargets(scada::NodeId{scada::id::Organizes}, true))
      .WillByDefault(Return(std::vector<NodeRef>{second, third}));

  TestExecutor executor;
  auto node_ids =
      WaitAwaitable(executor, ExpandGroupItemIdsAsync(executor, root,
                                                      /*max_count=*/2));

  EXPECT_THAT(node_ids, UnorderedElementsAre(first_id, second_id));
}

TEST(ClientUtilsTest, ExpandGroupItemIdsAsyncZeroLimitDoesNotFetch) {
  const scada::NodeId root_id{7100, 1};
  ModelNodeService node_service;
  auto root_model =
      MakeNodeModel(root_id, scada::NodeClass::Object, NodeFetchStatus::None);
  const NodeRef root = node_service.Add(root_id, root_model);
  EXPECT_CALL(*root_model, Fetch(_)).Times(0);

  TestExecutor executor;
  auto node_ids =
      WaitAwaitable(executor, ExpandGroupItemIdsAsync(executor, root,
                                                      /*max_count=*/0));

  EXPECT_TRUE(node_ids.empty());
}
