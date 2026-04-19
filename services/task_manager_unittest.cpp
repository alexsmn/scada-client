#include "task_manager_impl.h"

#include "address_space/test/test_scada_node_states.h"
#include "base/test/test_executor.h"
#include "core/progress_host_impl.h"
#include "events/local_events.h"
#include "model/data_items_node_ids.h"
#include "node_service/static/static_node_service.h"
#include "profile/profile.h"
#include "scada/attribute_service_mock.h"
#include "scada/node_management_service_mock.h"
#include "scada/status_exception.h"
#include "scada/status_promise.h"

#include <gmock/gmock.h>

#include "base/debug_util.h"

using namespace testing;

class TaskManagerTest : public Test {
 public:
  virtual void SetUp() override;

 protected:
  template <class T>
  T Wait(promise<T> pending) {
    using namespace std::chrono_literals;
    while (pending.wait_for(1ms) == promise_wait_status::timeout) {
      executor_->Advance(10ms);
    }
    executor_->Poll();
    return pending.get();
  }

  void Wait(promise<void> pending) {
    using namespace std::chrono_literals;
    while (pending.wait_for(1ms) == promise_wait_status::timeout) {
      executor_->Advance(10ms);
    }
    executor_->Poll();
    pending.get();
  }

  const std::shared_ptr<TestExecutor> executor_ =
      std::make_shared<TestExecutor>();

  StaticNodeService node_service_;
  scada::MockAttributeService attribute_service_;
  scada::MockNodeManagementService node_management_service_;
  LocalEvents local_events_;
  Profile profile_;
  ProgressHostImpl progress_host_;

  const std::shared_ptr<TaskManagerImpl> task_manager_ =
      std::make_shared<TaskManagerImpl>(TaskManagerImplContext{
          .executor_ = executor_,
          .node_service_ = node_service_,
          .attribute_service_ = attribute_service_,
          .node_management_service_ = node_management_service_,
          .local_events_ = local_events_,
          .profile_ = profile_,
          .progress_host_ = progress_host_});
};

void TaskManagerTest::SetUp() {
  node_service_.AddAll(GetScadaNodeStates());
}

TEST_F(TaskManagerTest, PostInsertTask_Succeeds) {
  const auto& parent_id = data_items::id::DataItems;
  const auto& type_def_id = data_items::id::DiscreteItemType;
  const auto& added_node_id = data_items::id::DataItems;

  EXPECT_CALL(
      node_management_service_,
      AddNodes(/*inputs=*/ElementsAre(FieldsAre(
                   /*requested_id=*/scada::NodeId{}, parent_id,
                   /*node_class=*/scada::NodeClass::Variable, type_def_id,
                   /*attributes=*/_)),
               /*callback=*/_))
      .WillOnce(InvokeArgument<1>(scada::StatusCode::Good,
                                  std::vector<scada::AddNodesResult>{
                                      {.added_node_id = added_node_id}}));

  const scada::NodeId node_id =
      Wait(task_manager_->PostInsertTask(
          // Provide the node ID similar to how it's provided on paste.
          {.node_id = scada::NodeId{1, NamespaceIndexes::TS},
           .type_definition_id = type_def_id,
           .parent_id = parent_id}));

  EXPECT_EQ(node_id, added_node_id);

  EXPECT_THAT(
      local_events_.events(),
      ElementsAre(Field(&scada::Event::severity, scada::kSeverityNormal)));

  EXPECT_FALSE(task_manager_->IsRunning());
}

TEST_F(TaskManagerTest, PostInsertTask_ServiceFails) {
  const auto& parent_id = data_items::id::DataItems;
  const auto& type_def_id = data_items::id::DiscreteItemType;

  EXPECT_CALL(
      node_management_service_,
      AddNodes(/*inputs=*/ElementsAre(FieldsAre(
                   /*requested_id=*/scada::NodeId{}, parent_id,
                   /*node_class=*/scada::NodeClass::Variable, type_def_id,
                   /*attributes=*/_)),
               /*callback=*/_))
      .WillOnce(InvokeArgument<1>(scada::StatusCode::Bad,
                                  std::vector<scada::AddNodesResult>{}));

  EXPECT_THROW(
      Wait(task_manager_->PostInsertTask({.type_definition_id = type_def_id,
                                          .parent_id = parent_id})),
      scada::status_exception);

  EXPECT_THAT(
      local_events_.events(),
      ElementsAre(Field(&scada::Event::severity, scada::kSeverityCritical)));

  EXPECT_FALSE(task_manager_->IsRunning());
}

TEST_F(TaskManagerTest, PostInsertTask_BadTypeDefId) {
  // Intentionally specify wrong type definition ID, so add node fails.
  EXPECT_THROW(
      Wait(task_manager_->PostInsertTask(
          {.type_definition_id = scada::id::References,
           .parent_id = data_items::id::DataItems})),
      scada::status_exception);

  EXPECT_THAT(
      local_events_.events(),
      ElementsAre(Field(&scada::Event::severity, scada::kSeverityCritical)));

  EXPECT_FALSE(task_manager_->IsRunning());
}

TEST_F(TaskManagerTest, PostDeleteTask_ServiceFails) {
  const auto& node_id = scada::NodeId{1, NamespaceIndexes::TS};

  EXPECT_CALL(node_management_service_,
              DeleteNodes(/*inputs=*/ElementsAre(
                              FieldsAre(node_id, /*delete_target_refs=*/false)),
                          /*callback=*/_))
      .WillOnce(InvokeArgument<1>(scada::StatusCode::Bad,
                                  std::vector<scada::StatusCode>{}));

  EXPECT_THROW(Wait(task_manager_->PostDeleteTask(node_id)),
               scada::status_exception);

  EXPECT_THAT(
      local_events_.events(),
      ElementsAre(Field(&scada::Event::severity, scada::kSeverityCritical)));

  EXPECT_FALSE(task_manager_->IsRunning());
}

TEST_F(TaskManagerTest, PostDeleteTask_Succeeds) {
  const auto& node_id = scada::NodeId{1, NamespaceIndexes::TS};

  EXPECT_CALL(node_management_service_,
              DeleteNodes(/*inputs=*/ElementsAre(
                              FieldsAre(node_id, /*delete_target_refs=*/false)),
                          /*callback=*/_))
      .WillOnce(InvokeArgument<1>(
          scada::StatusCode::Good,
          std::vector<scada::StatusCode>{scada::StatusCode::Good}));

  Wait(task_manager_->PostDeleteTask(node_id));

  // Profile::show_write_ok defaults to true, so successful deletes still emit
  // an info-severity event.
  EXPECT_THAT(local_events_.events(),
              ElementsAre(Field(&scada::Event::severity, scada::kSeverityNormal)));

  EXPECT_FALSE(task_manager_->IsRunning());
}

TEST_F(TaskManagerTest, PostAddReference_Succeeds) {
  const auto ref_type = scada::id::HasComponent;
  const auto src = scada::NodeId{1, NamespaceIndexes::TS};
  const auto dst = scada::NodeId{2, NamespaceIndexes::TS};

  // `AddReferencesCallback` is `std::function<void(Status&&, vector<StatusCode>&&)>`.
  // gmock's `InvokeArgument` cannot bind to the rvalue-reference parameters,
  // so we invoke the captured callback by hand.
  EXPECT_CALL(node_management_service_, AddReferences(_, _))
      .WillOnce(Invoke([&](const std::vector<scada::AddReferencesItem>& inputs,
                           const scada::AddReferencesCallback& callback) {
        ASSERT_EQ(inputs.size(), 1u);
        EXPECT_EQ(inputs[0].source_node_id, src);
        EXPECT_EQ(inputs[0].reference_type_id, ref_type);
        EXPECT_EQ(inputs[0].target_node_id, dst);
        callback(scada::StatusCode::Good, {scada::StatusCode::Good});
      }));

  Wait(task_manager_->PostAddReference(ref_type, src, dst));

  EXPECT_FALSE(task_manager_->IsRunning());
}

TEST_F(TaskManagerTest, PostAddReference_ServiceFails) {
  const auto ref_type = scada::id::HasComponent;
  const auto src = scada::NodeId{1, NamespaceIndexes::TS};
  const auto dst = scada::NodeId{2, NamespaceIndexes::TS};

  EXPECT_CALL(node_management_service_, AddReferences(_, _))
      .WillOnce(Invoke([](const std::vector<scada::AddReferencesItem>&,
                          const scada::AddReferencesCallback& callback) {
        callback(scada::StatusCode::Bad, {});
      }));

  EXPECT_THROW(Wait(task_manager_->PostAddReference(ref_type, src, dst)),
               scada::status_exception);

  EXPECT_THAT(
      local_events_.events(),
      ElementsAre(Field(&scada::Event::severity, scada::kSeverityCritical)));

  EXPECT_FALSE(task_manager_->IsRunning());
}

TEST_F(TaskManagerTest, PostDeleteReference_Succeeds) {
  const auto ref_type = scada::id::HasComponent;
  const auto src = scada::NodeId{1, NamespaceIndexes::TS};
  const auto dst = scada::NodeId{2, NamespaceIndexes::TS};

  EXPECT_CALL(node_management_service_, DeleteReferences(_, _))
      .WillOnce(Invoke(
          [&](const std::vector<scada::DeleteReferencesItem>& inputs,
              const scada::DeleteReferencesCallback& callback) {
            ASSERT_EQ(inputs.size(), 1u);
            EXPECT_EQ(inputs[0].source_node_id, src);
            EXPECT_EQ(inputs[0].reference_type_id, ref_type);
            EXPECT_EQ(inputs[0].target_node_id, dst);
            callback(scada::StatusCode::Good, {scada::StatusCode::Good});
          }));

  Wait(task_manager_->PostDeleteReference(ref_type, src, dst));

  EXPECT_FALSE(task_manager_->IsRunning());
}

TEST_F(TaskManagerTest, PostTask_LauncherFailurePropagates) {
  auto failing_launcher = []() -> promise<void> {
    return scada::MakeRejectedStatusPromise(scada::StatusCode::Bad_Disconnected);
  };

  EXPECT_THROW(Wait(task_manager_->PostTask(u"Custom", failing_launcher)),
               scada::status_exception);

  // The coroutine body reports a single failure event for the custom task.
  EXPECT_THAT(
      local_events_.events(),
      ElementsAre(Field(&scada::Event::severity, scada::kSeverityCritical)));

  EXPECT_FALSE(task_manager_->IsRunning());
}

TEST_F(TaskManagerTest, PostTask_LauncherSucceeds) {
  auto resolving_launcher = []() -> promise<void> {
    return make_resolved_promise();
  };

  Wait(task_manager_->PostTask(u"Custom", resolving_launcher));

  // Profile::show_write_ok defaults to true, so a successful custom task emits
  // an info-severity completion event.
  EXPECT_THAT(local_events_.events(),
              ElementsAre(Field(&scada::Event::severity, scada::kSeverityNormal)));

  EXPECT_FALSE(task_manager_->IsRunning());
}

// Regression: after the coroutine migration the queue must stay live across
// back-to-back `Post*Task` calls. Previously the whole PostInsertTask chain
// dispatched via `.then()`; now it runs as a single coroutine, and we want to
// guarantee a follow-up task still gets picked up by the queue.
TEST_F(TaskManagerTest, BackToBackPostDeleteTasksRunSequentially) {
  const auto& first = scada::NodeId{1, NamespaceIndexes::TS};
  const auto& second = scada::NodeId{2, NamespaceIndexes::TS};

  InSequence seq;
  EXPECT_CALL(node_management_service_,
              DeleteNodes(ElementsAre(FieldsAre(first, _)), _))
      .WillOnce(InvokeArgument<1>(
          scada::StatusCode::Good,
          std::vector<scada::StatusCode>{scada::StatusCode::Good}));
  EXPECT_CALL(node_management_service_,
              DeleteNodes(ElementsAre(FieldsAre(second, _)), _))
      .WillOnce(InvokeArgument<1>(
          scada::StatusCode::Good,
          std::vector<scada::StatusCode>{scada::StatusCode::Good}));

  auto first_promise = task_manager_->PostDeleteTask(first);
  auto second_promise = task_manager_->PostDeleteTask(second);

  Wait(std::move(first_promise));
  Wait(std::move(second_promise));

  EXPECT_FALSE(task_manager_->IsRunning());
}
