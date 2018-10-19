#pragma once

#include "core/configuration_types.h"
#include "core/node_attributes.h"
#include "core/status.h"

#include <functional>

class TaskManagerObserver {
 public:
  virtual ~TaskManagerObserver() {}

  struct Status {
    bool active = false;
    int range = 0;
    int current = 0;
  };

  virtual void OnTaskManagerStatus(const Status& status) = 0;
};

class TaskManager {
 public:
  virtual ~TaskManager() {}

  using InsertCallback =
      std::function<void(scada::Status status, const scada::NodeId& node_id)>;
  using UpdateCallback = std::function<void(scada::Status status)>;

  virtual void PostInsertTask(const scada::NodeId& requested_id,
                              const scada::NodeId& parent_id,
                              const scada::NodeId& type_id,
                              scada::NodeAttributes attributes,
                              scada::NodeProperties properties,
                              InsertCallback callback = {}) = 0;
  virtual void PostUpdateTask(const scada::NodeId& node_id,
                              scada::NodeAttributes attributes,
                              scada::NodeProperties properties,
                              UpdateCallback callback = {}) = 0;
  virtual void PostDeleteTask(const scada::NodeId& node_id) = 0;

  virtual void PostAddReference(const scada::NodeId& reference_type_id,
                                const scada::NodeId& source_id,
                                const scada::NodeId& target_id) = 0;
  virtual void PostDeleteReference(const scada::NodeId& reference_type_id,
                                   const scada::NodeId& source_id,
                                   const scada::NodeId& target_id) = 0;

  virtual void AddObserver(TaskManagerObserver& observer) = 0;
  virtual void RemoveObserver(TaskManagerObserver& observer) = 0;
};
