#pragma once

#include "scada/co_result.h"
#include "services/task_manager.h"

// TaskManager that refuses every write.
//
// The generator only ever renders dialogs and models; it never presses OK, so
// no task should be posted. Each method still rejects through a coroutine
// rather than aborting, so a capture that does reach a write path fails its
// own assertion instead of taking the process down.
class NullTaskManager : public TaskManager {
 public:
  scada::CoStatus PostTask(std::u16string_view, const TaskLauncher&) override {
    co_return scada::StatusCode::Bad;
  }
  scada::CoStatusOr<scada::NodeId> PostInsertTask(
      const scada::NodeState&) override {
    co_return scada::StatusCode::Bad;
  }
  scada::CoStatus PostUpdateTask(const scada::NodeId&,
                                 scada::NodeAttributes,
                                 scada::NodeProperties) override {
    co_return scada::StatusCode::Bad;
  }
  scada::CoStatus PostDeleteTask(const scada::NodeId&) override {
    co_return scada::StatusCode::Bad;
  }
  scada::CoStatus PostAddReference(const scada::NodeId&,
                                   const scada::NodeId&,
                                   const scada::NodeId&) override {
    co_return scada::StatusCode::Bad;
  }
  scada::CoStatus PostDeleteReference(const scada::NodeId&,
                                      const scada::NodeId&,
                                      const scada::NodeId&) override {
    co_return scada::StatusCode::Bad;
  }
};
