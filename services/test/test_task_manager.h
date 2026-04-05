#pragma once

#include "scada/status_promise.h"
#include "services/task_manager.h"
#include "services/test/test_storage.h"

class TestTaskManager : public TaskManager {
 public:
  explicit TestTaskManager(TestStorage& storage) : storage_{storage} {}

  virtual promise<> PostTask(std::u16string_view description,
                             const TaskLauncher& launcher) override {
    return scada::MakeRejectedStatusPromise(scada::StatusCode::Bad);
  }

  virtual promise<scada::NodeId> PostInsertTask(
      const scada::NodeState& node_state) override {
    assert(node_state.children.empty());

    auto node_state_copy = node_state;
    auto node_id = storage_.Insert(std::move(node_state_copy));
    return make_resolved_promise(std::move(node_id));
  }

  virtual promise<> PostUpdateTask(const scada::NodeId& node_id,
                                   scada::NodeAttributes attributes,
                                   scada::NodeProperties properties) override {
    return scada::MakeRejectedStatusPromise(scada::StatusCode::Bad);
  }

  virtual promise<> PostDeleteTask(const scada::NodeId& node_id) override {
    return scada::MakeRejectedStatusPromise(scada::StatusCode::Bad);
  }

  virtual promise<> PostAddReference(const scada::NodeId& reference_type_id,
                                     const scada::NodeId& source_id,
                                     const scada::NodeId& target_id) override {
    return scada::MakeRejectedStatusPromise(scada::StatusCode::Bad);
  }

  virtual promise<> PostDeleteReference(
      const scada::NodeId& reference_type_id,
      const scada::NodeId& source_id,
      const scada::NodeId& target_id) override {
    return scada::MakeRejectedStatusPromise(scada::StatusCode::Bad);
  }

  TestStorage& storage_;
};
