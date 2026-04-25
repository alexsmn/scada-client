#include "ui/common/client_utils.h"

#include "base/awaitable_promise.h"
#include "base/executor_conversions.h"
#include "base/test/test_executor.h"
#include "node_service/node_model_mock.h"

#include <gmock/gmock.h>

using namespace testing;
using namespace std::chrono_literals;

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
  return node_model;
}

template <class T>
T WaitForPromise(std::shared_ptr<TestExecutor> executor, promise<T> promise) {
  while (promise.wait_for(1ms) == promise_wait_status::timeout) {
    executor->Poll();
  }

  return promise.get();
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

  auto executor = std::make_shared<TestExecutor>();
  auto promise = ToPromise(MakeAnyExecutor(executor),
                           ExpandGroupItemIdsAsync(MakeAnyExecutor(executor),
                                                   NodeRef{root},
                                                   /*max_count=*/2));

  auto node_ids = WaitForPromise(executor, std::move(promise));

  EXPECT_THAT(node_ids, UnorderedElementsAre(first_id, second_id));
}

TEST(ClientUtilsTest, ExpandGroupItemIdsAsyncZeroLimitDoesNotFetch) {
  const scada::NodeId root_id{7100, 1};
  auto root = MakeNodeModel(root_id, scada::NodeClass::Object,
                            NodeFetchStatus::None());
  EXPECT_CALL(*root, Fetch(_, _)).Times(0);

  auto executor = std::make_shared<TestExecutor>();
  auto promise = ToPromise(MakeAnyExecutor(executor),
                           ExpandGroupItemIdsAsync(MakeAnyExecutor(executor),
                                                   NodeRef{root},
                                                   /*max_count=*/0));

  auto node_ids = WaitForPromise(executor, std::move(promise));

  EXPECT_TRUE(node_ids.empty());
}
