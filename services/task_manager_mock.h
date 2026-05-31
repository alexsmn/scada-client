#pragma once

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
  }

 private:
  static Awaitable<scada::StatusOr<scada::NodeId>> RejectPostInsertTaskAsync() {
    co_return scada::StatusCode::Bad;
  }

 public:
  MOCK_METHOD(Awaitable<scada::Status>,
              PostTask,
              (std::u16string_view description, const TaskLauncher& launcher),
              (override));

  MOCK_METHOD(Awaitable<scada::StatusOr<scada::NodeId>>,
              PostInsertTask,
              (const scada::NodeState& new_node_state),
              (override));

  MOCK_METHOD(Awaitable<scada::Status>,
              PostUpdateTask,
              (const scada::NodeId& node_id,
               scada::NodeAttributes attributes,
               scada::NodeProperties properties),
              (override));

  MOCK_METHOD(Awaitable<scada::Status>,
              PostDeleteTask,
              (const scada::NodeId& node_id),
              (override));

  MOCK_METHOD(Awaitable<scada::Status>,
              PostAddReference,
              (const scada::NodeId& reference_type_id,
               const scada::NodeId& source_id,
               const scada::NodeId& target_id),
              (override));

  MOCK_METHOD(Awaitable<scada::Status>,
              PostDeleteReference,
              (const scada::NodeId& reference_type_id,
               const scada::NodeId& source_id,
               const scada::NodeId& target_id),
              (override));
};
