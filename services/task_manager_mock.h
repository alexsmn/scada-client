#pragma once

#include "services/task_manager.h"

#include <gmock/gmock.h>

class MockTaskManager : public TaskManager {
 public:
  MOCK_METHOD(void,
              PostInsertTask,
              (const scada::NodeId& requested_id,
               const scada::NodeId& parent_id,
               const scada::NodeId& type_id,
               scada::NodeAttributes attributes,
               scada::NodeProperties properties,
               InsertCallback callback),
              (override));

  MOCK_METHOD(void,
              PostUpdateTask,
              (const scada::NodeId& node_id,
               scada::NodeAttributes attributes,
               scada::NodeProperties properties,
               UpdateCallback callback),
              (override));

  MOCK_METHOD(void, PostDeleteTask, (const scada::NodeId& node_id), (override));

  MOCK_METHOD(void,
              PostAddReference,
              (const scada::NodeId& reference_type_id,
               const scada::NodeId& source_id,
               const scada::NodeId& target_id),
              (override));

  MOCK_METHOD(void,
              PostDeleteReference,
              (const scada::NodeId& reference_type_id,
               const scada::NodeId& source_id,
               const scada::NodeId& target_id),
              (override));
};
