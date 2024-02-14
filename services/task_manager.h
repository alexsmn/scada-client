#pragma once

#include "common/node_state.h"
#include "scada/node_attributes.h"
#include "scada/status_promise.h"

#include <functional>

class TaskManager {
 public:
  virtual ~TaskManager() {}

  using TaskLauncher = std::function<scada::status_promise<void>()>;

  virtual scada::status_promise<void> PostTask(
      std::u16string_view description,
      const TaskLauncher& launcher) = 0;

  virtual promise<scada::NodeId> PostInsertTask(
      const scada::NodeId& requested_id,
      const scada::NodeId& parent_id,
      const scada::NodeId& type_id,
      scada::NodeAttributes attributes,
      scada::NodeProperties properties,
      std::vector<scada::ReferenceDescription> references) = 0;

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
