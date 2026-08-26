#include "limits/limit_model.h"

#include "aui/dialog_service.h"
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

// Records the boxes the model raises, so "the operator was told" is an
// observable fact rather than an inference from the status code.
class RecordingDialogService : public DialogService {
 public:
  UiView* GetDialogOwningWindow() const override { return nullptr; }
  UiView* GetParentWidget() const override { return nullptr; }

  Awaitable<MessageBoxResult> RunMessageBox(std::u16string_view message,
                                            std::u16string_view title,
                                            MessageBoxMode mode) override {
    messages.emplace_back(message);
    modes.emplace_back(mode);
    co_return MessageBoxResult::Ok;
  }

  Awaitable<std::filesystem::path> SelectOpenFile(
      std::u16string_view title) override {
    throw std::exception{};
    co_return std::filesystem::path{};
  }

  Awaitable<std::filesystem::path> SelectSaveFile(
      const SaveParams& params) override {
    throw std::exception{};
    co_return std::filesystem::path{};
  }

  std::vector<std::u16string> messages;
  std::vector<MessageBoxMode> modes;
};

class LimitModelTest : public Test {
 protected:
  LimitModelTest()
      : item_node_{node_service_.Add(scada::NodeState{
            .node_id = scada::NodeId{kItemNodeId, 1},
            .attributes = {.display_name = scada::LocalizedText{u"Current"}},
            .properties =
                {{scada::data_items::id::AnalogItemType_LimitLo, 10.0},
                 {scada::data_items::id::AnalogItemType_LimitHi, 90.0}}})},
        model_{std::make_shared<LimitModel>(
            LimitDialogContext{.executor_ = executor_,
                               .node_ = item_node_,
                               .task_manager_ = task_manager_})} {
    model_->set_dialog_service(&dialog_service_);
    model_->completion_handler = [this](bool ok) { completion_ = ok; };
  }

  void DrainExecutor() { Drain(executor_); }

  TestExecutor executor_;
  FakeNodeService node_service_;
  NodeRef item_node_;
  NiceMock<MockTaskManager> task_manager_;
  RecordingDialogService dialog_service_;
  // The model outlives the post it spawns — the dialog stays open until the
  // write reports — so it is shared_ptr-held and holds a weak_ptr internally.
  std::shared_ptr<LimitModel> model_;
  std::optional<bool> completion_;
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

  model_->WriteLimits({.lo = u"5", .hi = u"95", .lolo = u"1", .hihi = u"99"});
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

  model_->WriteLimits({.lo = u"5", .hi = u"95", .lolo = u"", .hihi = u"99"});
  DrainExecutor();

  EXPECT_TRUE(executed);
}

// A lazy awaitable that reports a refused write, so the model has a real
// failure status to react to rather than a fabricated one.
scada::CoStatus FailLazily(scada::StatusCode status_code) {
  co_return status_code;
}

// Regression for task 128: `WriteLimits` discarded the write's status, so a
// refused edit was indistinguishable from a written one — the dialog closed on
// both, and the bands the operator typed were silently not in effect.
TEST_F(LimitModelTest, RefusedWriteIsReportedToTheOperator) {
  EXPECT_CALL(task_manager_, PostUpdateTask(_, _, _))
      .WillOnce(Invoke([](const scada::NodeId&, scada::NodeAttributes,
                          scada::NodeProperties) {
        return FailLazily(scada::StatusCode::Bad_UserAccessDenied);
      }));

  model_->WriteLimits({.lo = u"5", .hi = u"95", .lolo = u"1", .hihi = u"99"});
  DrainExecutor();

  EXPECT_THAT(dialog_service_.messages, SizeIs(1));
  EXPECT_THAT(dialog_service_.modes, ElementsAre(MessageBoxMode::Error));
}

// The other half of the same entry, and the one an operator notices: the dialog
// must stay open on the values they typed. `completion_handler(false)` is what
// keeps it there — completing with true would close it, which is what the write
// dialog still does after reporting its own errors.
TEST_F(LimitModelTest, RefusedWriteLeavesTheDialogOpen) {
  EXPECT_CALL(task_manager_, PostUpdateTask(_, _, _))
      .WillOnce(Invoke([](const scada::NodeId&, scada::NodeAttributes,
                          scada::NodeProperties) {
        return FailLazily(scada::StatusCode::Bad_UserAccessDenied);
      }));

  model_->WriteLimits({.lo = u"5", .hi = u"95", .lolo = u"1", .hihi = u"99"});
  DrainExecutor();

  ASSERT_TRUE(completion_.has_value());
  EXPECT_FALSE(*completion_);
}

// A written edit still closes the dialog, and reports nothing. Without this the
// fix above could "pass" by never completing at all, which would strand the
// operator on a dialog whose Apply button no longer does anything.
TEST_F(LimitModelTest, SuccessfulWriteCompletesWithoutReporting) {
  bool executed = false;
  EXPECT_CALL(task_manager_, PostUpdateTask(_, _, _))
      .WillOnce(Invoke([&executed](const scada::NodeId&, scada::NodeAttributes,
                                   scada::NodeProperties) {
        return CompleteLazily(&executed);
      }));

  model_->WriteLimits({.lo = u"5", .hi = u"95", .lolo = u"1", .hihi = u"99"});
  DrainExecutor();

  ASSERT_TRUE(completion_.has_value());
  EXPECT_TRUE(*completion_);
  EXPECT_THAT(dialog_service_.messages, IsEmpty());
}

// Apply held down must not queue four writes of the same bands: a second post
// while the first is in flight would race two completions onto one dialog.
TEST_F(LimitModelTest, SecondApplyWhileWritingIsIgnored) {
  EXPECT_CALL(task_manager_, PostUpdateTask(_, _, _))
      .WillOnce(Invoke([](const scada::NodeId&, scada::NodeAttributes,
                          scada::NodeProperties) {
        return FailLazily(scada::StatusCode::Bad_UserAccessDenied);
      }));

  const LimitModel::Limits limits{
      .lo = u"5", .hi = u"95", .lolo = u"1", .hihi = u"99"};
  model_->WriteLimits(limits);
  model_->WriteLimits(limits);
  DrainExecutor();

  EXPECT_THAT(dialog_service_.messages, SizeIs(1));
}

}  // namespace
