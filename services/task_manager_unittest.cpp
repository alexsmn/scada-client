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

#include <gmock/gmock.h>

#include "base/debug_util-inl.h"

using namespace testing;

class TaskManagerTest : public Test {
 public:
  virtual void SetUp() override;

 protected:
  const std::shared_ptr<Executor> executor_ = std::make_shared<TestExecutor>();

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

  auto node_id = task_manager_
                     ->PostInsertTask({.type_definition_id = type_def_id,
                                       .parent_id = parent_id})
                     .get();

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

  EXPECT_THROW(task_manager_
                   ->PostInsertTask({.type_definition_id = type_def_id,
                                     .parent_id = parent_id})
                   .get(),
               scada::status_exception);

  EXPECT_THAT(
      local_events_.events(),
      ElementsAre(Field(&scada::Event::severity, scada::kSeverityCritical)));

  EXPECT_FALSE(task_manager_->IsRunning());
}

TEST_F(TaskManagerTest, PostInsertTask_BadTypeDefId) {
  // Intentionally specify wrong type definition ID, so add node fails.
  EXPECT_THROW(
      task_manager_
          ->PostInsertTask({.type_definition_id = scada::id::References,
                            .parent_id = data_items::id::DataItems})
          .get(),
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

  EXPECT_THROW(task_manager_->PostDeleteTask(node_id).get(),
               scada::status_exception);

  EXPECT_THAT(
      local_events_.events(),
      ElementsAre(Field(&scada::Event::severity, scada::kSeverityCritical)));

  EXPECT_FALSE(task_manager_->IsRunning());
}
