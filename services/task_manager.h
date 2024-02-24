#pragma once

#include "common/node_state.h"
#include "scada/status_promise.h"

#include <functional>

class TaskManager {
 public:
  virtual ~TaskManager() {}

  using TaskLauncher = std::function<scada::status_promise<void>()>;

  virtual scada::status_promise<void> PostTask(
      std::u16string_view description,
      const TaskLauncher& launcher) = 0;

  // Those fields must be unset: node_class, reference_type_id.
  virtual scada::status_promise<scada::NodeId> PostInsertTask(
      const scada::NodeState& new_node_state) = 0;

  virtual scada::status_promise<void> PostUpdateTask(
      const scada::NodeId& node_id,
      scada::NodeAttributes attributes,
      scada::NodeProperties properties) = 0;

  virtual scada::status_promise<void> PostDeleteTask(
      const scada::NodeId& node_id) = 0;

  virtual scada::status_promise<void> PostAddReference(
      const scada::NodeId& reference_type_id,
      const scada::NodeId& source_id,
      const scada::NodeId& target_id) = 0;

  virtual scada::status_promise<void> PostDeleteReference(
      const scada::NodeId& reference_type_id,
      const scada::NodeId& source_id,
      const scada::NodeId& target_id) = 0;
};
