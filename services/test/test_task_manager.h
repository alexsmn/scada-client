#pragma once

#include "services/task_manager.h"
#include "services/test/test_storage.h"

class TestTaskManager : public TaskManager {
 public:
  explicit TestTaskManager(TestStorage& storage) : storage_{storage} {}

  virtual Awaitable<scada::Status> PostTask(
      std::u16string_view description,
      const TaskLauncher& launcher) override {
    co_return scada::StatusCode::Bad;
  }

  virtual Awaitable<scada::StatusOr<scada::NodeId>> PostInsertTask(
      const scada::NodeState& node_state) override {
    assert(node_state.children.empty());

    auto node_state_copy = node_state;
    auto node_id = storage_.Insert(std::move(node_state_copy));
    co_return node_id;
  }

  virtual Awaitable<scada::Status> PostUpdateTask(
      const scada::NodeId& node_id,
      scada::NodeAttributes attributes,
      scada::NodeProperties properties) override {
    co_return scada::StatusCode::Bad;
  }

  virtual Awaitable<scada::Status> PostDeleteTask(
      const scada::NodeId& node_id) override {
    co_return scada::StatusCode::Bad;
  }

  virtual Awaitable<scada::Status> PostAddReference(
      const scada::NodeId& reference_type_id,
      const scada::NodeId& source_id,
      const scada::NodeId& target_id) override {
    co_return scada::StatusCode::Bad;
  }

  virtual Awaitable<scada::Status> PostDeleteReference(
      const scada::NodeId& reference_type_id,
      const scada::NodeId& source_id,
      const scada::NodeId& target_id) override {
    co_return scada::StatusCode::Bad;
  }

  TestStorage& storage_;
};
