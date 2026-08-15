#include "limits/limit_model.h"

#include "base/test/awaitable_test.h"
#include "base/test/test_executor.h"
#include "common/node_state.h"
#include "model/data_items_node_ids.h"
#include "node_service/test/fake_node_service.h"
#include "scada/co_result.h"
#include "services/task_manager_mock.h"

#include <gmock/gmock.h>

using namespace testing;

namespace {

constexpr scada::NumericId kItemNodeId = 7001;

// A lazy awaitable that flips `executed` only when actually awaited — mirrors
// the laziness of the production `TaskManagerImpl`, so a discarded return
// value leaves `executed` false. Same helper as
// `configuration_module_unittest.cpp`'s.
scada::CoStatus CompleteLazily(bool* executed) {
  *executed = true;
  co_return scada::StatusCode::Good;
}

class LimitModelTest : public Test {
 protected:
  LimitModelTest()
      : item_node_{node_service_.Add(scada::NodeState{
            .node_id = scada::NodeId{kItemNodeId, 1},
            .attributes = {.display_name = scada::LocalizedText{u"Current"}},
            .properties =
                {{scada::data_items::id::AnalogItemType_LimitLo, 10.0},
                 {scada::data_items::id::AnalogItemType_LimitHi, 90.0}}})},
        model_{LimitDialogContext{.executor_ = executor_,
                                  .node_ = item_node_,
                                  .task_manager_ = task_manager_}} {}

  void DrainExecutor() { Drain(executor_); }

  TestExecutor executor_;
  FakeNodeService node_service_;
  NodeRef item_node_;
  NiceMock<MockTaskManager> task_manager_;
  LimitModel model_;
};

// Regression for the defect this fixture was written for: `WriteLimits`
// discarded the lazy awaitable returned by `TaskManager::PostUpdateTask`, so
// the operator's edits were never posted — the dialog closed as if they had
// been written. Asserting on the mock call alone does not catch it: the call
// happens either way, and only awaiting the result runs the task.
TEST_F(LimitModelTest, WriteLimitsRunsPostedUpdateTask) {
  bool executed = false;

  EXPECT_CALL(task_manager_,
              PostUpdateTask(scada::NodeId{kItemNodeId, 1}, _, _))
      .WillOnce(Invoke([&executed](const scada::NodeId&, scada::NodeAttributes,
                                   scada::NodeProperties properties) {
        EXPECT_THAT(properties,
                    UnorderedElementsAre(
                        Pair(scada::data_items::id::AnalogItemType_LimitLo,
                             scada::Variant{5.0}),
                        Pair(scada::data_items::id::AnalogItemType_LimitHi,
                             scada::Variant{95.0}),
                        Pair(scada::data_items::id::AnalogItemType_LimitLoLo,
                             scada::Variant{1.0}),
                        Pair(scada::data_items::id::AnalogItemType_LimitHiHi,
                             scada::Variant{99.0})));
        return CompleteLazily(&executed);
      }));

  model_.WriteLimits({.lo = u"5", .hi = u"95", .lolo = u"1", .hihi = u"99"});
  DrainExecutor();

  EXPECT_TRUE(executed);
}

// An emptied field clears the band rather than writing a zero, and that must
// survive the spawn too — the properties are moved into the coroutine frame.
TEST_F(LimitModelTest, WriteLimitsClearsEmptiedBands) {
  bool executed = false;

  EXPECT_CALL(task_manager_, PostUpdateTask(_, _, _))
      .WillOnce(Invoke([&executed](const scada::NodeId&, scada::NodeAttributes,
                                   scada::NodeProperties properties) {
        EXPECT_THAT(
            properties,
            Contains(Pair(scada::data_items::id::AnalogItemType_LimitLoLo,
                          scada::Variant{})));
        return CompleteLazily(&executed);
      }));

  model_.WriteLimits({.lo = u"5", .hi = u"95", .lolo = u"", .hihi = u"99"});
  DrainExecutor();

  EXPECT_TRUE(executed);
}

}  // namespace
