#pragma once

#include "scada/co_result.h"
#include "services/task_manager.h"

#include <gmock/gmock.h>

class MockTaskManager : public TaskManager {
 public:
  MockTaskManager() {
    using namespace testing;

    ON_CALL(*this, PostInsertTask(/*new_node_state=*/_))
        .WillByDefault([](const scada::NodeState&) {
          return RejectPostInsertTaskAsync();
        });

    // Every `CoStatus`-returning method needs a default action too. Without
    // one, gmock answers an unstubbed call with a default-constructed
    // `boost::asio::awaitable` — a null frame whose `await_ready()` still
    // returns false, so `co_await`ing it dereferences null inside the
    // *awaiting* coroutine, with this mock named nowhere in the backtrace
    // (task 698; CLAUDE.md, "Unit Test Guidance"). A mock nobody stubbed did
    // not post the task, so it answers `Bad_NotSupported`.
    ON_CALL(*this, PostTask(/*description=*/_, /*launcher=*/_))
        .WillByDefault([](std::u16string_view, const TaskLauncher&) {
          return RejectAsync();
        });

    ON_CALL(*this, PostUpdateTask(/*node_id=*/_, /*attributes=*/_,
                                  /*properties=*/_))
        .WillByDefault([](const scada::NodeId&, scada::NodeAttributes,
                          scada::NodeProperties) { return RejectAsync(); });

    ON_CALL(*this, PostDeleteTask(/*node_id=*/_))
        .WillByDefault([](const scada::NodeId&) { return RejectAsync(); });

    ON_CALL(*this, PostAddReference(/*reference_type_id=*/_, /*source_id=*/_,
                                    /*target_id=*/_))
        .WillByDefault([](const scada::NodeId&, const scada::NodeId&,
                          const scada::NodeId&) { return RejectAsync(); });

    ON_CALL(*this, PostDeleteReference(/*reference_type_id=*/_, /*source_id=*/_,
                                       /*target_id=*/_))
        .WillByDefault([](const scada::NodeId&, const scada::NodeId&,
                          const scada::NodeId&) { return RejectAsync(); });
  }

 private:
  static scada::CoStatusOr<scada::NodeId> RejectPostInsertTaskAsync() {
    co_return scada::StatusCode::Bad;
  }

  static scada::CoStatus RejectAsync() {
    co_return scada::StatusCode::Bad_NotSupported;
  }

 public:
  MOCK_METHOD(scada::CoStatus,
              PostTask,
              (std::u16string_view description, const TaskLauncher& launcher),
              (override));

  MOCK_METHOD(scada::CoStatusOr<scada::NodeId>,
              PostInsertTask,
              (const scada::NodeState& new_node_state),
              (override));

  MOCK_METHOD(scada::CoStatus,
              PostUpdateTask,
              (const scada::NodeId& node_id,
               scada::NodeAttributes attributes,
               scada::NodeProperties properties),
              (override));

  MOCK_METHOD(scada::CoStatus,
              PostDeleteTask,
              (const scada::NodeId& node_id),
              (override));

  MOCK_METHOD(scada::CoStatus,
              PostAddReference,
              (const scada::NodeId& reference_type_id,
               const scada::NodeId& source_id,
               const scada::NodeId& target_id),
              (override));

  MOCK_METHOD(scada::CoStatus,
              PostDeleteReference,
              (const scada::NodeId& reference_type_id,
               const scada::NodeId& source_id,
               const scada::NodeId& target_id),
              (override));
};
