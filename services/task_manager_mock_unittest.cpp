#include "services/task_manager_mock.h"

#include "base/test/awaitable_test.h"
#include "base/test/test_executor.h"
#include "scada/co_result.h"
#include "scada/node_id.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <utility>

using namespace testing;

namespace {

// Every unstubbed `CoStatus`-returning method completes with a bad status
// rather than with gmock's default-constructed awaitable, which holds a null
// frame whose `await_ready()` still returns false — so `co_await`ing it
// dereferences null inside the *awaiting* coroutine, with this mock named
// nowhere in the backtrace (task 698; CLAUDE.md, "Unit Test Guidance").
//
// `NiceMock` is the case that matters: a `StrictMock` reports the unexpected
// call before it can return anything, so it never reaches the null frame.
class TaskManagerMockTest : public Test {
 protected:
  void ExpectRejects(scada::CoStatus pending) {
    scada::Status status = WaitAwaitable(executor_, std::move(pending));
    EXPECT_TRUE(status.bad());
  }

  TestExecutor executor_;
  NiceMock<MockTaskManager> task_manager_;
};

TEST_F(TaskManagerMockTest, UnstubbedPostTaskRejects) {
  ExpectRejects(
      task_manager_.PostTask(u"description", TaskManager::TaskLauncher{}));
}

TEST_F(TaskManagerMockTest, UnstubbedPostUpdateTaskRejects) {
  ExpectRejects(task_manager_.PostUpdateTask(scada::NodeId{1}, {}, {}));
}

TEST_F(TaskManagerMockTest, UnstubbedPostDeleteTaskRejects) {
  ExpectRejects(task_manager_.PostDeleteTask(scada::NodeId{1}));
}

TEST_F(TaskManagerMockTest, UnstubbedPostAddReferenceRejects) {
  ExpectRejects(task_manager_.PostAddReference(
      scada::NodeId{1}, scada::NodeId{2}, scada::NodeId{3}));
}

TEST_F(TaskManagerMockTest, UnstubbedPostDeleteReferenceRejects) {
  ExpectRejects(task_manager_.PostDeleteReference(
      scada::NodeId{1}, scada::NodeId{2}, scada::NodeId{3}));
}

}  // namespace
