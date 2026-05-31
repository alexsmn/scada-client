#include "ui/common/client_utils.h"

#include "base/any_executor.h"
#include "base/test/awaitable_test.h"
#include "base/test/test_executor.h"
#include "node_service/node_model_mock.h"

#include <gmock/gmock.h>

using namespace testing;
namespace {

std::shared_ptr<NiceMock<MockNodeModel>> MakeNodeModel(
    const scada::NodeId& node_id,
    scada::NodeClass node_class,
    NodeFetchStatus fetch_status = NodeFetchStatus::NodeAndChildren()) {
  auto node_model = std::make_shared<NiceMock<MockNodeModel>>();
  ON_CALL(*node_model, GetFetchStatus()).WillByDefault(Return(fetch_status));
  ON_CALL(*node_model, GetStatus())
      .WillByDefault(Return(scada::StatusCode::Good));
  ON_CALL(*node_model, GetAttribute(scada::AttributeId::NodeId))
      .WillByDefault(Return(node_id));
  ON_CALL(*node_model, GetAttribute(scada::AttributeId::NodeClass))
      .WillByDefault(Return(static_cast<scada::Int32>(node_class)));
  ON_CALL(*node_model, Fetch(_))
      .WillByDefault([](const NodeFetchStatus&) -> Awaitable<void> {
        co_return;
      });
  return node_model;
}

}  // namespace

TEST(ClientUtilsTest, ExpandGroupItemIdsAsyncRespectsMaxCount) {
  const scada::NodeId root_id{7000, 1};
  const scada::NodeId first_id{7001, 1};
  const scada::NodeId group_id{7002, 1};
  const scada::NodeId second_id{7003, 1};
  const scada::NodeId third_id{7004, 1};

  auto root = MakeNodeModel(root_id, scada::NodeClass::Object);
  auto first = MakeNodeModel(first_id, scada::NodeClass::Variable);
  auto group = MakeNodeModel(group_id, scada::NodeClass::Object);
  auto second = MakeNodeModel(second_id, scada::NodeClass::Variable);
  auto third = MakeNodeModel(third_id, scada::NodeClass::Variable);

  ON_CALL(*root, GetTargets(scada::NodeId{scada::id::Organizes}, true))
      .WillByDefault(Return(std::vector<NodeRef>{NodeRef{first}}));
  ON_CALL(*root, GetTargets(scada::NodeId{scada::id::HasComponent}, true))
      .WillByDefault(Return(std::vector<NodeRef>{NodeRef{group}}));
  ON_CALL(*group, GetTargets(scada::NodeId{scada::id::Organizes}, true))
      .WillByDefault(Return(std::vector<NodeRef>{NodeRef{second},
                                                NodeRef{third}}));

  TestExecutor executor;
  auto node_ids =
      WaitAwaitable(executor,
                    ExpandGroupItemIdsAsync(executor,
                                            NodeRef{root},
                                            /*max_count=*/2));

  EXPECT_THAT(node_ids, UnorderedElementsAre(first_id, second_id));
}

TEST(ClientUtilsTest, ExpandGroupItemIdsAsyncZeroLimitDoesNotFetch) {
  const scada::NodeId root_id{7100, 1};
  auto root = MakeNodeModel(root_id, scada::NodeClass::Object,
                            NodeFetchStatus::None());
  EXPECT_CALL(*root, Fetch(_)).Times(0);

  TestExecutor executor;
  auto node_ids =
      WaitAwaitable(executor,
                    ExpandGroupItemIdsAsync(executor,
                                            NodeRef{root},
                                            /*max_count=*/0));

  EXPECT_TRUE(node_ids.empty());
}
