#pragma once

#include "base/promise.h"
#include "common/node_state.h"
#include "scada/node_attributes.h"
#include "scada/status.h"

#include <functional>

class TaskManager {
 public:
  virtual ~TaskManager() {}

  using TaskLauncher = std::function<promise<>()>;

  virtual promise<> PostTask(std::u16string_view description,
                             const TaskLauncher& launcher) = 0;

  virtual promise<scada::NodeId> PostInsertTask(
      const scada::NodeId& requested_id,
      const scada::NodeId& parent_id,
      const scada::NodeId& type_id,
      scada::NodeAttributes attributes,
      scada::NodeProperties properties,
      std::vector<scada::ReferenceDescription> references) = 0;

  virtual promise<> PostUpdateTask(const scada::NodeId& node_id,
                                   scada::NodeAttributes attributes,
                                   scada::NodeProperties properties) = 0;

  virtual promise<> PostDeleteTask(const scada::NodeId& node_id) = 0;

  virtual promise<> PostAddReference(const scada::NodeId& reference_type_id,
                                     const scada::NodeId& source_id,
                                     const scada::NodeId& target_id) = 0;

  virtual promise<> PostDeleteReference(const scada::NodeId& reference_type_id,
                                        const scada::NodeId& source_id,
                                        const scada::NodeId& target_id) = 0;
};
