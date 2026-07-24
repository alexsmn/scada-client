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
  }

 private:
  static scada::CoStatusOr<scada::NodeId> RejectPostInsertTaskAsync() {
    co_return scada::StatusCode::Bad;
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
