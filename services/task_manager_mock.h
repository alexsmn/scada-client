#pragma once

#include "services/task_manager.h"

#include <gmock/gmock.h>

class MockTaskManager : public TaskManager {
 public:
  MockTaskManager() {
    using namespace testing;

    ON_CALL(*this, PostInsertTask(/*new_node_state=*/_))
        .WillByDefault(
            Return(make_rejected_promise<scada::NodeId>(std::exception{})));
  }

  MOCK_METHOD(promise<>,
              PostTask,
              (std::u16string_view description, const TaskLauncher& launcher),
              (override));

  MOCK_METHOD(promise<scada::NodeId>,
              PostInsertTask,
              (const scada::NodeState& new_node_state),
              (override));

  MOCK_METHOD(promise<>,
              PostUpdateTask,
              (const scada::NodeId& node_id,
               scada::NodeAttributes attributes,
               scada::NodeProperties properties),
              (override));

  MOCK_METHOD(promise<>,
              PostDeleteTask,
              (const scada::NodeId& node_id),
              (override));

  MOCK_METHOD(promise<>,
              PostAddReference,
              (const scada::NodeId& reference_type_id,
               const scada::NodeId& source_id,
               const scada::NodeId& target_id),
              (override));

  MOCK_METHOD(promise<>,
              PostDeleteReference,
              (const scada::NodeId& reference_type_id,
               const scada::NodeId& source_id,
               const scada::NodeId& target_id),
              (override));
};
