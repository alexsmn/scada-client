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
  static Awaitable<scada::NodeId> RejectPostInsertTaskAsync() {
    throw std::exception{};
    co_return scada::NodeId{};
  }

 public:
  MOCK_METHOD(Awaitable<void>,
              PostTask,
              (std::u16string_view description, const TaskLauncher& launcher),
              (override));

  MOCK_METHOD(Awaitable<scada::NodeId>,
              PostInsertTask,
              (const scada::NodeState& new_node_state),
              (override));

  MOCK_METHOD(Awaitable<void>,
              PostUpdateTask,
              (const scada::NodeId& node_id,
               scada::NodeAttributes attributes,
               scada::NodeProperties properties),
              (override));

  MOCK_METHOD(Awaitable<void>,
              PostDeleteTask,
              (const scada::NodeId& node_id),
              (override));

  MOCK_METHOD(Awaitable<void>,
              PostAddReference,
              (const scada::NodeId& reference_type_id,
               const scada::NodeId& source_id,
               const scada::NodeId& target_id),
              (override));

  MOCK_METHOD(Awaitable<void>,
              PostDeleteReference,
              (const scada::NodeId& reference_type_id,
               const scada::NodeId& source_id,
               const scada::NodeId& target_id),
              (override));
};
