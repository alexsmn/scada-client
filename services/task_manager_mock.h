#pragma once

#include "services/task_manager.h"

#include <gmock/gmock.h>

class MockTaskManager : public TaskManager {
 public:
  MOCK_METHOD(promise<scada::NodeId>,
              PostInsertTask,
              (const scada::NodeId& requested_id,
               const scada::NodeId& parent_id,
               const scada::NodeId& type_id,
               scada::NodeAttributes attributes,
               scada::NodeProperties properties,
               std::vector<scada::ReferenceDescription> references),
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
