#pragma once

#include "scada/status_exception.h"
#include "services/task_manager.h"
#include "services/test/test_storage.h"

class TestTaskManager : public TaskManager {
 public:
  explicit TestTaskManager(TestStorage& storage) : storage_{storage} {}

  virtual Awaitable<void> PostTask(std::u16string_view description,
                                   const TaskLauncher& launcher) override {
    throw scada::status_exception{scada::StatusCode::Bad};
    co_return;
  }

  virtual Awaitable<scada::NodeId> PostInsertTask(
      const scada::NodeState& node_state) override {
    assert(node_state.children.empty());

    auto node_state_copy = node_state;
    auto node_id = storage_.Insert(std::move(node_state_copy));
    co_return node_id;
  }

  virtual Awaitable<void> PostUpdateTask(
      const scada::NodeId& node_id,
      scada::NodeAttributes attributes,
      scada::NodeProperties properties) override {
    throw scada::status_exception{scada::StatusCode::Bad};
    co_return;
  }

  virtual Awaitable<void> PostDeleteTask(const scada::NodeId& node_id) override {
    throw scada::status_exception{scada::StatusCode::Bad};
    co_return;
  }

  virtual Awaitable<void> PostAddReference(
      const scada::NodeId& reference_type_id,
      const scada::NodeId& source_id,
      const scada::NodeId& target_id) override {
    throw scada::status_exception{scada::StatusCode::Bad};
    co_return;
  }

  virtual Awaitable<void> PostDeleteReference(
      const scada::NodeId& reference_type_id,
      const scada::NodeId& source_id,
      const scada::NodeId& target_id) override {
    throw scada::status_exception{scada::StatusCode::Bad};
    co_return;
  }

  TestStorage& storage_;
};
