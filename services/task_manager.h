#pragma once

#include "base/awaitable.h"
#include "common/node_state.h"
#include "scada/co_result.h"
#include "scada/status.h"
#include "scada/status_or.h"

#include <functional>

// Queues user-visible configuration tasks (inserts, updates, deletes,
// reference changes) and runs them sequentially, reporting progress and
// completion to the user.
//
// Contract: every `Post*` method enqueues the task BEFORE returning; the
// returned awaitable only waits for the task's result. Callers that don't
// need the result may discard the awaitable — the task still runs
// (fire-and-forget). Implementations must not defer the enqueue into the
// returned (lazy) awaitable: many UI call sites discard it, and a lazy
// implementation silently turns them into no-ops.
class TaskManager {
 public:
  virtual ~TaskManager() {}

  using TaskLauncher = std::function<scada::CoStatus()>;

  virtual scada::CoStatus PostTask(std::u16string_view description,
                                   const TaskLauncher& launcher) = 0;

  // Those fields must be unset: node_class, reference_type_id, children.
  virtual scada::CoStatusOr<scada::NodeId> PostInsertTask(
      const scada::NodeState& new_node_state) = 0;

  virtual scada::CoStatus PostUpdateTask(const scada::NodeId& node_id,
                                         scada::NodeAttributes attributes,
                                         scada::NodeProperties properties) = 0;

  virtual scada::CoStatus PostDeleteTask(const scada::NodeId& node_id) = 0;

  virtual scada::CoStatus PostAddReference(
      const scada::NodeId& reference_type_id,
      const scada::NodeId& source_id,
      const scada::NodeId& target_id) = 0;

  virtual scada::CoStatus PostDeleteReference(
      const scada::NodeId& reference_type_id,
      const scada::NodeId& source_id,
      const scada::NodeId& target_id) = 0;
};
