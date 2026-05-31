#pragma once

#include "base/awaitable.h"
#include "common/node_state.h"
#include "scada/status.h"
#include "scada/status_or.h"

#include <functional>

class TaskManager {
 public:
  virtual ~TaskManager() {}

  using TaskLauncher = std::function<Awaitable<scada::Status>()>;

  virtual Awaitable<scada::Status> PostTask(
      std::u16string_view description,
      const TaskLauncher& launcher) = 0;

  // Those fields must be unset: node_class, reference_type_id, children.
  virtual Awaitable<scada::StatusOr<scada::NodeId>> PostInsertTask(
      const scada::NodeState& new_node_state) = 0;

  virtual Awaitable<scada::Status> PostUpdateTask(
      const scada::NodeId& node_id,
      scada::NodeAttributes attributes,
      scada::NodeProperties properties) = 0;

  virtual Awaitable<scada::Status> PostDeleteTask(
      const scada::NodeId& node_id) = 0;

  virtual Awaitable<scada::Status> PostAddReference(
      const scada::NodeId& reference_type_id,
      const scada::NodeId& source_id,
      const scada::NodeId& target_id) = 0;

  virtual Awaitable<scada::Status> PostDeleteReference(
      const scada::NodeId& reference_type_id,
      const scada::NodeId& source_id,
      const scada::NodeId& target_id) = 0;
};
