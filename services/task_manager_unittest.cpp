#include "task_manager_impl.h"

#include "address_space/test/scada_test_address_space.h"
#include "address_space/test/test_scada_node_states.h"
#include "base/test/awaitable_test.h"
#include "base/test/test_executor.h"
#include "common/formula_util.h"
#include "core/progress_host_impl.h"
#include "events/local_events.h"
#include "events/view_events_subscription.h"
#include "model/data_items_node_ids.h"
#include "model/devices_node_ids.h"
#include "model/namespaces.h"
#include "model/node_id_util.h"
#include "node_service/node_awaitable.h"
#include "node_service/static/static_node_service.h"
#include "node_service/v3/node_fetcher.h"
#include "node_service/v3/node_service_impl.h"
#include "profile/profile.h"
#include "scada/attribute_service_mock.h"
#include "scada/monitored_item_service_mock.h"
#include "scada/node_management_service_mock.h"

#include <gmock/gmock.h>

#include <map>

#include "base/debug_util.h"
#include "scada/co_result.h"

using namespace testing;

class TaskManagerTest : public Test {
 public:
  virtual void SetUp() override;

 protected:
  template <class T>
  T Wait(Awaitable<T> pending) {
    auto result = StartAwaitable(executor_, std::move(pending));
    while (!result->done) {
      executor_.Advance(10ms);
      Drain(executor_);
    }
    executor_.Poll();
    return WaitResult(executor_, std::move(result));
  }

  void Wait(Awaitable<void> pending) {
    auto result = StartAwaitable(executor_, std::move(pending));
    while (!result->done) {
      executor_.Advance(10ms);
      Drain(executor_);
    }
    executor_.Poll();
    WaitResult(executor_, std::move(result));
  }

  TestExecutor executor_;

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
  const auto& parent_id = scada::data_items::id::DataItems;
  const auto& type_def_id = scada::data_items::id::DiscreteItemType;
  const auto& added_node_id = scada::data_items::id::DataItems;

  EXPECT_CALL(
      node_management_service_,
      AddNodes(/*context=*/_, /*inputs=*/ElementsAre(FieldsAre(
                   /*requested_id=*/scada::NodeId{}, parent_id,
                   /*node_class=*/scada::NodeClass::Variable, type_def_id,
                   /*attributes=*/_))))
      .WillOnce([&](scada::ServiceContext, std::vector<scada::AddNodesItem>)
                    -> Awaitable<
                        scada::StatusOr<std::vector<scada::AddNodesResult>>> {
        return scada::MakeNodeManagementResult<scada::AddNodesResult>(
            std::vector<scada::AddNodesResult>{
                {.added_node_id = added_node_id}});
      });

  auto node_id = Wait(task_manager_->PostInsertTask(
      // Provide the node ID similar to how it's provided on paste.
      {.node_id = scada::NodeId{1, scada::NamespaceIndexes::TS},
       .type_definition_id = type_def_id,
       .parent_id = parent_id}));

  ASSERT_TRUE(node_id.ok());
  EXPECT_EQ(*node_id, added_node_id);

  EXPECT_THAT(
      local_events_.events(),
      ElementsAre(Field(&scada::Event::severity, scada::kSeverityNormal)));

  EXPECT_FALSE(task_manager_->IsRunning());
}

// Regression: references of a freshly inserted node were silently dropped —
// `RunInsertTask` discarded the lazy awaitables returned by
// `PostAddReference`, so the reference tasks were never queued or executed.
TEST_F(TaskManagerTest, PostInsertTask_AddsReferences) {
  const auto& parent_id = scada::data_items::id::DataItems;
  const auto& type_def_id = scada::data_items::id::DiscreteItemType;
  const scada::NodeId added_node_id{7, scada::NamespaceIndexes::TS};
  const scada::NodeId forward_target{8, scada::NamespaceIndexes::TS};
  const scada::NodeId inverse_source{9, scada::NamespaceIndexes::TS};

  EXPECT_CALL(node_management_service_, AddNodes(_, _))
      .WillOnce([&](scada::ServiceContext, std::vector<scada::AddNodesItem>)
                    -> Awaitable<
                        scada::StatusOr<std::vector<scada::AddNodesResult>>> {
        return scada::MakeNodeManagementResult<scada::AddNodesResult>(
            std::vector<scada::AddNodesResult>{
                {.added_node_id = added_node_id}});
      });

  EXPECT_CALL(node_management_service_, AddReferences(_, _))
      .WillOnce([&](scada::ServiceContext,
                    std::vector<scada::AddReferencesItem> inputs)
                    -> scada::CoStatusOr<std::vector<scada::StatusCode>> {
        EXPECT_EQ(inputs.size(), 2u);
        // The forward reference goes from the added node to its target.
        EXPECT_EQ(inputs[0].source_node_id, added_node_id);
        EXPECT_EQ(inputs[0].reference_type_id, scada::id::Organizes);
        EXPECT_EQ(inputs[0].target_node_id.node_id(), forward_target);
        // The inverse reference goes from its source to the added node.
        EXPECT_EQ(inputs[1].source_node_id, inverse_source);
        EXPECT_EQ(inputs[1].reference_type_id, scada::id::Organizes);
        EXPECT_EQ(inputs[1].target_node_id.node_id(), added_node_id);
        return scada::MakeNodeManagementResult<scada::StatusCode>(
            std::vector{scada::StatusCode::Good, scada::StatusCode::Good});
      });

  auto node_id = Wait(task_manager_->PostInsertTask(
      {.type_definition_id = type_def_id,
       .parent_id = parent_id,
       .references = {{.reference_type_id = scada::id::Organizes,
                       .forward = true,
                       .node_id = forward_target},
                      {.reference_type_id = scada::id::Organizes,
                       .forward = false,
                       .node_id = inverse_source}}}));

  ASSERT_TRUE(node_id.ok());
  EXPECT_EQ(*node_id, added_node_id);

  EXPECT_FALSE(task_manager_->IsRunning());
}

TEST_F(TaskManagerTest, PostInsertTask_AddReferencesFailurePropagates) {
  const auto& parent_id = scada::data_items::id::DataItems;
  const auto& type_def_id = scada::data_items::id::DiscreteItemType;
  const scada::NodeId added_node_id{7, scada::NamespaceIndexes::TS};

  EXPECT_CALL(node_management_service_, AddNodes(_, _))
      .WillOnce([&](scada::ServiceContext, std::vector<scada::AddNodesItem>)
                    -> Awaitable<
                        scada::StatusOr<std::vector<scada::AddNodesResult>>> {
        return scada::MakeNodeManagementResult<scada::AddNodesResult>(
            std::vector<scada::AddNodesResult>{
                {.added_node_id = added_node_id}});
      });

  EXPECT_CALL(node_management_service_, AddReferences(_, _))
      .WillOnce([](scada::ServiceContext, std::vector<scada::AddReferencesItem>)
                    -> scada::CoStatusOr<std::vector<scada::StatusCode>> {
        return scada::MakeNodeManagementResult<scada::StatusCode>(
            std::vector{scada::StatusCode::Bad_WrongNodeId});
      });

  auto node_id = Wait(task_manager_->PostInsertTask(
      {.type_definition_id = type_def_id,
       .parent_id = parent_id,
       .references = {
           {.reference_type_id = scada::id::Organizes,
            .forward = true,
            .node_id = scada::NodeId{8, scada::NamespaceIndexes::TS}}}}));

  ASSERT_FALSE(node_id.ok());
  EXPECT_EQ(node_id.status().code(), scada::StatusCode::Bad_WrongNodeId);

  EXPECT_FALSE(task_manager_->IsRunning());
}

TEST_F(TaskManagerTest, PostInsertTask_ServiceFails) {
  const auto& parent_id = scada::data_items::id::DataItems;
  const auto& type_def_id = scada::data_items::id::DiscreteItemType;

  EXPECT_CALL(
      node_management_service_,
      AddNodes(/*context=*/_, /*inputs=*/ElementsAre(FieldsAre(
                   /*requested_id=*/scada::NodeId{}, parent_id,
                   /*node_class=*/scada::NodeClass::Variable, type_def_id,
                   /*attributes=*/_))))
      .WillOnce([](scada::ServiceContext, std::vector<scada::AddNodesItem>)
                    -> Awaitable<
                        scada::StatusOr<std::vector<scada::AddNodesResult>>> {
        return scada::MakeNodeManagementResult<scada::AddNodesResult>(
            scada::Status{scada::StatusCode::Bad});
      });

  auto result = Wait(task_manager_->PostInsertTask(
      {.type_definition_id = type_def_id, .parent_id = parent_id}));
  ASSERT_FALSE(result.ok());
  EXPECT_EQ(result.status().code(), scada::StatusCode::Bad);

  EXPECT_THAT(
      local_events_.events(),
      ElementsAre(Field(&scada::Event::severity, scada::kSeverityCritical)));

  EXPECT_FALSE(task_manager_->IsRunning());
}

TEST_F(TaskManagerTest, PostInsertTask_BadTypeDefId) {
  // Intentionally specify wrong type definition ID, so add node fails.
  auto result = Wait(task_manager_->PostInsertTask(
      {.type_definition_id = scada::id::References,
       .parent_id = scada::data_items::id::DataItems}));
  ASSERT_FALSE(result.ok());
  EXPECT_EQ(result.status().code(), scada::StatusCode::Bad_WrongTypeId);

  EXPECT_THAT(
      local_events_.events(),
      ElementsAre(Field(&scada::Event::severity, scada::kSeverityCritical)));

  EXPECT_FALSE(task_manager_->IsRunning());
}

// Backlog 1015: a 2.5.7 Server's TransmissionItemType has no SourceNode, so
// inserting a retransmission object used to create the object and then fail
// writing its source -- an object with no source, reported as "Wrong property
// identifier". A property the Server's type does not declare is now refused
// before anything is created, and named as a Server version question.
TEST_F(TaskManagerTest, PostInsertTask_UndeclaredPropertyCreatesNothing) {
  EXPECT_CALL(node_management_service_, AddNodes(_, _)).Times(0);

  // An id this Server has no node for stands in for SourceNode on 2.5.7.
  const scada::NodeId unknown_declaration{987654,
                                          scada::NamespaceIndexes::SCADA};
  auto result = Wait(task_manager_->PostInsertTask(
      {.type_definition_id = scada::data_items::id::DiscreteItemType,
       .parent_id = scada::data_items::id::DataItems,
       .properties = {{unknown_declaration, scada::Variant{true}}}}));
  ASSERT_FALSE(result.ok());
  EXPECT_EQ(result.status().code(), scada::StatusCode::Bad_WrongPropertyId);

  ASSERT_THAT(
      local_events_.events(),
      ElementsAre(Field(&scada::Event::severity, scada::kSeverityCritical)));
  EXPECT_NE(local_events_.events()[0]->message.text.find(u"2.6"),
            std::u16string::npos);
  EXPECT_FALSE(task_manager_->IsRunning());
}

// The gate passes a property the type does declare, through its supertype.
TEST_F(TaskManagerTest, PostInsertTask_DeclaredPropertyReachesTheServer) {
  EXPECT_CALL(node_management_service_, AddNodes(_, _))
      .WillOnce([](scada::ServiceContext, std::vector<scada::AddNodesItem>)
                    -> Awaitable<
                        scada::StatusOr<std::vector<scada::AddNodesResult>>> {
        return scada::MakeNodeManagementResult<scada::AddNodesResult>(
            scada::Status{scada::StatusCode::Bad});
      });

  auto result = Wait(task_manager_->PostInsertTask(
      {.type_definition_id = scada::data_items::id::DiscreteItemType,
       .parent_id = scada::data_items::id::DataItems,
       .properties = {{scada::data_items::id::DataItemType_Alias,
                       scada::Variant{scada::String{"a"}}}}}));
  ASSERT_FALSE(result.ok());
  EXPECT_EQ(result.status().code(), scada::StatusCode::Bad);
}

TEST_F(TaskManagerTest, PostDeleteTask_ServiceFails) {
  const auto& node_id = scada::NodeId{1, scada::NamespaceIndexes::TS};

  EXPECT_CALL(node_management_service_,
              DeleteNodes(/*context=*/_, /*inputs=*/ElementsAre(FieldsAre(
                              node_id, /*delete_target_refs=*/false))))
      .WillOnce([](scada::ServiceContext, std::vector<scada::DeleteNodesItem>)
                    -> scada::CoStatusOr<std::vector<scada::StatusCode>> {
        return scada::MakeNodeManagementResult<scada::StatusCode>(
            scada::Status{scada::StatusCode::Bad});
      });

  auto status = Wait(task_manager_->PostDeleteTask(node_id));
  EXPECT_EQ(status.code(), scada::StatusCode::Bad);

  EXPECT_THAT(
      local_events_.events(),
      ElementsAre(Field(&scada::Event::severity, scada::kSeverityCritical)));

  EXPECT_FALSE(task_manager_->IsRunning());
}

TEST_F(TaskManagerTest, PostDeleteTask_Succeeds) {
  const auto& node_id = scada::NodeId{1, scada::NamespaceIndexes::TS};

  EXPECT_CALL(node_management_service_,
              DeleteNodes(/*context=*/_, /*inputs=*/ElementsAre(FieldsAre(
                              node_id, /*delete_target_refs=*/false))))
      .WillOnce([](scada::ServiceContext, std::vector<scada::DeleteNodesItem>)
                    -> scada::CoStatusOr<std::vector<scada::StatusCode>> {
        return scada::MakeNodeManagementResult<scada::StatusCode>(
            std::vector{scada::StatusCode::Good});
      });

  auto status = Wait(task_manager_->PostDeleteTask(node_id));
  EXPECT_TRUE(status);

  // Profile::show_write_ok defaults to true, so successful deletes still emit
  // an info-severity event.
  EXPECT_THAT(
      local_events_.events(),
      ElementsAre(Field(&scada::Event::severity, scada::kSeverityNormal)));

  EXPECT_FALSE(task_manager_->IsRunning());
}

// Regression for the TaskManager contract: `Post*` methods must enqueue the
// task before returning, so call sites that discard the returned awaitable
// (fire-and-forget UI commands, e.g. property edits, transmission edits,
// recursive deletes) still run. When `PostTaskMethod` was a lazy coroutine,
// every such call site silently became a no-op.
TEST_F(TaskManagerTest, PostDeleteTask_RunsWhenAwaitableIsDiscarded) {
  const auto& node_id = scada::NodeId{1, scada::NamespaceIndexes::TS};

  bool deleted = false;
  EXPECT_CALL(node_management_service_, DeleteNodes(_, _))
      .WillOnce([&](scada::ServiceContext, std::vector<scada::DeleteNodesItem>)
                    -> scada::CoStatusOr<std::vector<scada::StatusCode>> {
        deleted = true;
        return scada::MakeNodeManagementResult<scada::StatusCode>(
            std::vector{scada::StatusCode::Good});
      });

  {
    auto discarded = task_manager_->PostDeleteTask(node_id);
    // Destroy the awaitable without ever awaiting it.
  }

  // The task manager schedules its queue pump with real-time delayed tasks,
  // so pace the polling loop on wall-clock time like `WaitResult` does.
  const auto deadline = std::chrono::steady_clock::now() + 5s;
  while ((!deleted || task_manager_->IsRunning()) &&
         std::chrono::steady_clock::now() < deadline) {
    executor_.Advance(10ms);
    Drain(executor_);
    std::this_thread::yield();
  }

  EXPECT_TRUE(deleted);
  EXPECT_FALSE(task_manager_->IsRunning());
}

TEST_F(TaskManagerTest, PostAddReference_Succeeds) {
  const auto ref_type = scada::id::HasComponent;
  const auto src = scada::NodeId{1, scada::NamespaceIndexes::TS};
  const auto dst = scada::NodeId{2, scada::NamespaceIndexes::TS};

  EXPECT_CALL(node_management_service_, AddReferences(_, _))
      .WillOnce([&](scada::ServiceContext,
                    std::vector<scada::AddReferencesItem> inputs)
                    -> scada::CoStatusOr<std::vector<scada::StatusCode>> {
        EXPECT_EQ(inputs.size(), 1u);
        EXPECT_EQ(inputs[0].source_node_id, src);
        EXPECT_EQ(inputs[0].reference_type_id, ref_type);
        EXPECT_EQ(inputs[0].target_node_id, dst);
        return scada::MakeNodeManagementResult<scada::StatusCode>(
            std::vector{scada::StatusCode::Good});
      });

  auto status = Wait(task_manager_->PostAddReference(ref_type, src, dst));
  EXPECT_TRUE(status);

  EXPECT_FALSE(task_manager_->IsRunning());
}

TEST_F(TaskManagerTest, PostAddReference_ServiceFails) {
  const auto ref_type = scada::id::HasComponent;
  const auto src = scada::NodeId{1, scada::NamespaceIndexes::TS};
  const auto dst = scada::NodeId{2, scada::NamespaceIndexes::TS};

  EXPECT_CALL(node_management_service_, AddReferences(_, _))
      .WillOnce([](scada::ServiceContext, std::vector<scada::AddReferencesItem>)
                    -> scada::CoStatusOr<std::vector<scada::StatusCode>> {
        return scada::MakeNodeManagementResult<scada::StatusCode>(
            scada::Status{scada::StatusCode::Bad});
      });

  auto status = Wait(task_manager_->PostAddReference(ref_type, src, dst));
  EXPECT_EQ(status.code(), scada::StatusCode::Bad);

  EXPECT_THAT(
      local_events_.events(),
      ElementsAre(Field(&scada::Event::severity, scada::kSeverityCritical)));

  EXPECT_FALSE(task_manager_->IsRunning());
}

TEST_F(TaskManagerTest, PostDeleteReference_Succeeds) {
  const auto ref_type = scada::id::HasComponent;
  const auto src = scada::NodeId{1, scada::NamespaceIndexes::TS};
  const auto dst = scada::NodeId{2, scada::NamespaceIndexes::TS};

  EXPECT_CALL(node_management_service_, DeleteReferences(_, _))
      .WillOnce([&](scada::ServiceContext,
                    std::vector<scada::DeleteReferencesItem> inputs)
                    -> scada::CoStatusOr<std::vector<scada::StatusCode>> {
        EXPECT_EQ(inputs.size(), 1u);
        EXPECT_EQ(inputs[0].source_node_id, src);
        EXPECT_EQ(inputs[0].reference_type_id, ref_type);
        EXPECT_EQ(inputs[0].target_node_id, dst);
        return scada::MakeNodeManagementResult<scada::StatusCode>(
            std::vector{scada::StatusCode::Good});
      });

  auto status = Wait(task_manager_->PostDeleteReference(ref_type, src, dst));
  EXPECT_TRUE(status);

  EXPECT_FALSE(task_manager_->IsRunning());
}

TEST_F(TaskManagerTest, PostTask_LauncherFailurePropagates) {
  auto failing_launcher = []() -> scada::CoStatus {
    co_return scada::StatusCode::Bad_Disconnected;
  };

  auto status = Wait(task_manager_->PostTask(u"Custom", failing_launcher));
  EXPECT_EQ(status.code(), scada::StatusCode::Bad_Disconnected);

  // The coroutine body reports a single failure event for the custom task.
  EXPECT_THAT(
      local_events_.events(),
      ElementsAre(Field(&scada::Event::severity, scada::kSeverityCritical)));

  EXPECT_FALSE(task_manager_->IsRunning());
}

TEST_F(TaskManagerTest, PostTask_LauncherSucceeds) {
  auto resolving_launcher = []() -> scada::CoStatus {
    co_return scada::StatusCode::Good;
  };

  auto status = Wait(task_manager_->PostTask(u"Custom", resolving_launcher));
  EXPECT_TRUE(status);

  // Profile::show_write_ok defaults to true, so a successful custom task emits
  // an info-severity completion event.
  EXPECT_THAT(
      local_events_.events(),
      ElementsAre(Field(&scada::Event::severity, scada::kSeverityNormal)));

  EXPECT_FALSE(task_manager_->IsRunning());
}

// Regression: after the coroutine migration the queue must stay live across
// back-to-back `Post*Task` calls. Previously the whole PostInsertTask chain
// dispatched via `.then()`; now it runs as a single coroutine, and we want to
// guarantee a follow-up task still gets picked up by the queue.
TEST_F(TaskManagerTest, BackToBackPostDeleteTasksRunSequentially) {
  const auto& first = scada::NodeId{1, scada::NamespaceIndexes::TS};
  const auto& second = scada::NodeId{2, scada::NamespaceIndexes::TS};

  InSequence seq;
  EXPECT_CALL(node_management_service_,
              DeleteNodes(_, ElementsAre(FieldsAre(first, _))))
      .WillOnce([](scada::ServiceContext, std::vector<scada::DeleteNodesItem>)
                    -> scada::CoStatusOr<std::vector<scada::StatusCode>> {
        return scada::MakeNodeManagementResult<scada::StatusCode>(
            std::vector{scada::StatusCode::Good});
      });
  EXPECT_CALL(node_management_service_,
              DeleteNodes(_, ElementsAre(FieldsAre(second, _))))
      .WillOnce([](scada::ServiceContext, std::vector<scada::DeleteNodesItem>)
                    -> scada::CoStatusOr<std::vector<scada::StatusCode>> {
        return scada::MakeNodeManagementResult<scada::StatusCode>(
            std::vector{scada::StatusCode::Good});
      });

  auto first_task = task_manager_->PostDeleteTask(first);
  auto second_task = task_manager_->PostDeleteTask(second);

  EXPECT_TRUE(Wait(std::move(first_task)));
  EXPECT_TRUE(Wait(std::move(second_task)));

  EXPECT_FALSE(task_manager_->IsRunning());
}

// --- "Two-stage control" follows the device (backlog 1016) -----------------
//
// The default used to be derived only by the bulk-create wizard and the multi
// create, so an object created one at a time, dropped from an IEC 61850
// device, or later re-bound to a MODBUS / IEC 60870-5 device kept Yes and the
// Server refused its select. The task manager now decides it on every insert
// and every channel edit.

class TaskManagerTwoStagedTest : public TaskManagerTest {
 protected:
  void SetUp() override {
    TaskManagerTest::SetUp();
    node_service_.Add(
        {.node_id = kModbusDevice,
         .node_class = scada::NodeClass::Object,
         .type_definition_id = scada::devices::id::ModbusDeviceType});
    node_service_.Add(
        {.node_id = kIec104Device,
         .node_class = scada::NodeClass::Object,
         .type_definition_id = scada::devices::id::Iec60870DeviceType});
    node_service_.Add(
        {.node_id = kIec61850Device,
         .node_class = scada::NodeClass::Object,
         .type_definition_id = scada::devices::id::Iec61850DeviceType});
    // An IEC 61850 data object is a real node under its device.
    node_service_.Add({.node_id = kIec61850DataObject,
                       .node_class = scada::NodeClass::Variable,
                       .type_definition_id = scada::id::BaseVariableType,
                       .parent_id = kIec61850Device,
                       .reference_type_id = scada::id::Organizes});

    ON_CALL(attribute_service_, Write(_, _))
        .WillByDefault([this](scada::ServiceContext,
                              std::vector<scada::WriteValue> inputs) {
          writes_.insert(writes_.end(), inputs.begin(), inputs.end());
          return scada::MakeNodeManagementResult<scada::StatusCode>(
              std::vector<scada::StatusCode>(inputs.size(),
                                             scada::StatusCode::Good));
        });
  }

  // An existing data item with the given type and properties.
  void AddItem(const scada::NodeId& type_id, scada::NodeProperties properties) {
    node_service_.Add({.node_id = kItem,
                       .node_class = scada::NodeClass::Variable,
                       .type_definition_id = type_id,
                       .parent_id = scada::data_items::id::DataItems,
                       .reference_type_id = scada::id::Organizes,
                       .properties = std::move(properties)});
  }

  static std::string Channel(const scada::NodeId& device_id,
                             std::string_view address) {
    return MakeNodeIdFormula(scada::MakeNestedNodeId(device_id, address));
  }

  // The value written to the item's "Two-stage control", if any.
  std::optional<bool> WrittenTwoStaged() {
    const scada::NodeId prop_id =
        node_service_
            .GetNode(kItem)[scada::data_items::id::DataItemType_OutputTwoStaged]
            .node_id();
    std::optional<bool> result;
    for (const scada::WriteValue& write : writes_) {
      if (write.node_id == prop_id)
        result = write.value.get_or(true);
    }
    return result;
  }

  void ExpectInsertedAs(const scada::NodeId& type_id) {
    EXPECT_CALL(node_management_service_, AddNodes(_, _))
        .WillOnce([](scada::ServiceContext, std::vector<scada::AddNodesItem>) {
          return scada::MakeNodeManagementResult<scada::AddNodesResult>(
              std::vector<scada::AddNodesResult>{{.added_node_id = kItem}});
        });
    // What the Server holds once the node exists: every property, with the
    // nodeset's default Yes for two-stage control.
    AddItem(type_id,
            {{scada::data_items::id::DataItemType_Input1, scada::String{}},
             {scada::data_items::id::DataItemType_Output, scada::String{}},
             {scada::data_items::id::DataItemType_OutputTwoStaged, true}});
  }

  static inline const scada::NodeId kModbusDevice{901,
                                                  scada::NamespaceIndexes::TS};
  static inline const scada::NodeId kIec104Device{902,
                                                  scada::NamespaceIndexes::TS};
  static inline const scada::NodeId kItem{903, scada::NamespaceIndexes::TS};
  static inline const scada::NodeId kIec61850Device{
      904, scada::NamespaceIndexes::TS};
  static inline const scada::NodeId kIec61850DataObject{
      905, scada::NamespaceIndexes::TS};

  // A Classic OPC item address: not a node under a device, but a string id in
  // the OPC namespace.
  static std::string OpcChannel(std::string_view item) {
    return MakeNodeIdFormula(scada::NodeId{
        "Vendor.Server.1\\" + std::string{item}, scada::NamespaceIndexes::OPC});
  }

  std::vector<scada::WriteValue> writes_;
};

// The single *Create* path and a channel drop both insert one node bound to a
// channel and leave the property unset; it must come out one-stage on MODBUS.
TEST_F(TaskManagerTwoStagedTest, InsertOnModbusChannelIsOneStage) {
  ExpectInsertedAs(scada::data_items::id::DiscreteItemType);

  auto node_id = Wait(task_manager_->PostInsertTask(
      {.type_definition_id = scada::data_items::id::DiscreteItemType,
       .parent_id = scada::data_items::id::DataItems,
       .properties = {{scada::data_items::id::DataItemType_Output,
                       Channel(kModbusDevice, "1")}}}));

  ASSERT_TRUE(node_id.ok());
  EXPECT_EQ(WrittenTwoStaged(), false);
}

// A caller that states the value (the wizards, a paste) is not overruled.
TEST_F(TaskManagerTwoStagedTest, InsertKeepsAnExplicitValue) {
  ExpectInsertedAs(scada::data_items::id::AnalogItemType);

  auto node_id = Wait(task_manager_->PostInsertTask(
      {.type_definition_id = scada::data_items::id::AnalogItemType,
       .parent_id = scada::data_items::id::DataItems,
       .properties = {
           {scada::data_items::id::DataItemType_Input1,
            Channel(kModbusDevice, "1")},
           {scada::data_items::id::DataItemType_OutputTwoStaged, true}}}));

  ASSERT_TRUE(node_id.ok());
  EXPECT_EQ(WrittenTwoStaged(), true);
}

// A discrete item on IEC 60870-5 selects, so the default stays Yes and nothing
// extra is written.
TEST_F(TaskManagerTwoStagedTest, InsertOnSelectableChannelKeepsTheDefault) {
  ExpectInsertedAs(scada::data_items::id::DiscreteItemType);

  auto node_id = Wait(task_manager_->PostInsertTask(
      {.type_definition_id = scada::data_items::id::DiscreteItemType,
       .parent_id = scada::data_items::id::DataItems,
       .properties = {{scada::data_items::id::DataItemType_Output,
                       Channel(kIec104Device, "1")}}}));

  ASSERT_TRUE(node_id.ok());
  EXPECT_EQ(WrittenTwoStaged(), std::nullopt);
}

// An object created unbound holds the default Yes; binding it to a MODBUS
// device afterwards turns it off.
TEST_F(TaskManagerTwoStagedTest, BindingAnUnboundItemToModbusTurnsItOff) {
  AddItem(scada::data_items::id::DiscreteItemType,
          {{scada::data_items::id::DataItemType_Input1, scada::String{}},
           {scada::data_items::id::DataItemType_Output, scada::String{}},
           {scada::data_items::id::DataItemType_OutputTwoStaged, true}});

  auto status = Wait(task_manager_->PostUpdateTask(
      kItem, {},
      {{scada::data_items::id::DataItemType_Output,
        Channel(kModbusDevice, "5")}}));

  EXPECT_TRUE(status);
  EXPECT_EQ(WrittenTwoStaged(), false);
}

// The value was the MODBUS default (No); moving the item to a device that
// selects restores the default Yes.
TEST_F(TaskManagerTwoStagedTest, RebindingFromModbusRestoresTheDefault) {
  AddItem(scada::data_items::id::DiscreteItemType,
          {{scada::data_items::id::DataItemType_Input1, scada::String{}},
           {scada::data_items::id::DataItemType_Output,
            Channel(kModbusDevice, "5")},
           {scada::data_items::id::DataItemType_OutputTwoStaged, false}});

  auto status = Wait(task_manager_->PostUpdateTask(
      kItem, {},
      {{scada::data_items::id::DataItemType_Output,
        Channel(kIec104Device, "5")}}));

  EXPECT_TRUE(status);
  EXPECT_EQ(WrittenTwoStaged(), true);
}

// Yes on a MODBUS item is not the MODBUS default, so somebody chose it; a
// re-bind leaves it alone.
TEST_F(TaskManagerTwoStagedTest, RebindingKeepsAValueTheEngineerChose) {
  AddItem(scada::data_items::id::DiscreteItemType,
          {{scada::data_items::id::DataItemType_Input1, scada::String{}},
           {scada::data_items::id::DataItemType_Output,
            Channel(kModbusDevice, "5")},
           {scada::data_items::id::DataItemType_OutputTwoStaged, true}});

  auto status = Wait(task_manager_->PostUpdateTask(
      kItem, {},
      {{scada::data_items::id::DataItemType_Output,
        Channel(kIec104Device, "5")}}));

  EXPECT_TRUE(status);
  EXPECT_EQ(WrittenTwoStaged(), std::nullopt);
}

// A new address on the same device is not a device change.
TEST_F(TaskManagerTwoStagedTest, ReaddressingOnTheSameDeviceWritesNothing) {
  AddItem(scada::data_items::id::DiscreteItemType,
          {{scada::data_items::id::DataItemType_Input1, scada::String{}},
           {scada::data_items::id::DataItemType_Output,
            Channel(kModbusDevice, "5")},
           {scada::data_items::id::DataItemType_OutputTwoStaged, true}});

  auto status = Wait(task_manager_->PostUpdateTask(
      kItem, {},
      {{scada::data_items::id::DataItemType_Output,
        Channel(kModbusDevice, "6")}}));

  EXPECT_TRUE(status);
  EXPECT_EQ(WrittenTwoStaged(), std::nullopt);
}

// --- Backlog 1036: the IEC 60870-5 type override ---------------------------

// A discrete item whose Output is a `FLOAT:` set-point is created one-stage:
// the Server refuses to select a set-point.
TEST_F(TaskManagerTwoStagedTest, InsertOnIec104SetPointOverrideIsOneStage) {
  ExpectInsertedAs(scada::data_items::id::DiscreteItemType);

  auto node_id = Wait(task_manager_->PostInsertTask(
      {.type_definition_id = scada::data_items::id::DiscreteItemType,
       .parent_id = scada::data_items::id::DataItems,
       .properties = {{scada::data_items::id::DataItemType_Output,
                       Channel(kIec104Device, "FLOAT:5")}}}));

  ASSERT_TRUE(node_id.ok());
  EXPECT_EQ(WrittenTwoStaged(), false);
}

// Adding the override on the same device changes the command, so a value
// still at the old default follows it.
TEST_F(TaskManagerTwoStagedTest, AddingASetPointOverrideTurnsItOff) {
  AddItem(scada::data_items::id::DiscreteItemType,
          {{scada::data_items::id::DataItemType_Input1, scada::String{}},
           {scada::data_items::id::DataItemType_Output,
            Channel(kIec104Device, "5")},
           {scada::data_items::id::DataItemType_OutputTwoStaged, true}});

  auto status = Wait(task_manager_->PostUpdateTask(
      kItem, {},
      {{scada::data_items::id::DataItemType_Output,
        Channel(kIec104Device, "INT16:5")}}));

  EXPECT_TRUE(status);
  EXPECT_EQ(WrittenTwoStaged(), false);
}

// An analog item (default No) given a `BOOL:` override now sends `C_SC_NA_1`,
// which selects: the default Yes comes back.
TEST_F(TaskManagerTwoStagedTest, ABoolOverrideOnAnAnalogItemRestoresYes) {
  AddItem(scada::data_items::id::AnalogItemType,
          {{scada::data_items::id::DataItemType_Input1, scada::String{}},
           {scada::data_items::id::DataItemType_Output,
            Channel(kIec104Device, "5")},
           {scada::data_items::id::DataItemType_OutputTwoStaged, false}});

  auto status = Wait(task_manager_->PostUpdateTask(
      kItem, {},
      {{scada::data_items::id::DataItemType_Output,
        Channel(kIec104Device, "BOOL:5")}}));

  EXPECT_TRUE(status);
  EXPECT_EQ(WrittenTwoStaged(), true);
}

// --- Backlog 1028: OPC and the unknown IEC 61850 ----------------------------
//
// OPC said nothing, so an object created on it kept Yes, and a MODBUS item
// moved to it or to an IEC 61850 direct-control object turned back to Yes —
// all of which the Server refuses to select.

// The OPC tier refuses every select, so an OPC object starts one-stage.
TEST_F(TaskManagerTwoStagedTest, InsertOnOpcChannelIsOneStage) {
  ExpectInsertedAs(scada::data_items::id::DiscreteItemType);

  auto node_id = Wait(task_manager_->PostInsertTask(
      {.type_definition_id = scada::data_items::id::DiscreteItemType,
       .parent_id = scada::data_items::id::DataItems,
       .properties = {{scada::data_items::id::DataItemType_Output,
                       OpcChannel("Group.Breaker1")}}}));

  ASSERT_TRUE(node_id.ok());
  EXPECT_EQ(WrittenTwoStaged(), false);
}

// An unbound object (default Yes) bound to OPC turns off.
TEST_F(TaskManagerTwoStagedTest, BindingAnUnboundItemToOpcTurnsItOff) {
  AddItem(scada::data_items::id::DiscreteItemType,
          {{scada::data_items::id::DataItemType_Input1, scada::String{}},
           {scada::data_items::id::DataItemType_Output, scada::String{}},
           {scada::data_items::id::DataItemType_OutputTwoStaged, true}});

  auto status = Wait(task_manager_->PostUpdateTask(
      kItem, {},
      {{scada::data_items::id::DataItemType_Output,
        OpcChannel("Group.Breaker1")}}));

  EXPECT_TRUE(status);
  EXPECT_EQ(WrittenTwoStaged(), false);
}

// MODBUS → OPC: both one-stage, so the No stays and nothing is written.
TEST_F(TaskManagerTwoStagedTest, RebindingFromModbusToOpcStaysOneStage) {
  AddItem(scada::data_items::id::DiscreteItemType,
          {{scada::data_items::id::DataItemType_Input1, scada::String{}},
           {scada::data_items::id::DataItemType_Output,
            Channel(kModbusDevice, "5")},
           {scada::data_items::id::DataItemType_OutputTwoStaged, false}});

  auto status = Wait(task_manager_->PostUpdateTask(
      kItem, {},
      {{scada::data_items::id::DataItemType_Output,
        OpcChannel("Group.Breaker1")}}));

  EXPECT_TRUE(status);
  EXPECT_EQ(WrittenTwoStaged(), std::nullopt);
}

// MODBUS → IEC 61850: whether the object selects is on the IED, so the
// Client does not know, and keeps the No rather than risk an object every
// select of which is refused.
TEST_F(TaskManagerTwoStagedTest, RebindingFromModbusToIec61850KeepsOneStage) {
  AddItem(scada::data_items::id::DiscreteItemType,
          {{scada::data_items::id::DataItemType_Input1, scada::String{}},
           {scada::data_items::id::DataItemType_Output,
            Channel(kModbusDevice, "5")},
           {scada::data_items::id::DataItemType_OutputTwoStaged, false}});

  auto status = Wait(task_manager_->PostUpdateTask(
      kItem, {},
      {{scada::data_items::id::DataItemType_Output,
        MakeNodeIdFormula(kIec61850DataObject)}}));

  EXPECT_TRUE(status);
  EXPECT_EQ(WrittenTwoStaged(), std::nullopt);
}

// IEC 61850 creation is unchanged: an object dropped from an IEC 61850 device
// keeps the default Yes (991af9bbd).
TEST_F(TaskManagerTwoStagedTest, InsertOnIec61850ChannelKeepsTheDefault) {
  ExpectInsertedAs(scada::data_items::id::DiscreteItemType);

  auto node_id = Wait(task_manager_->PostInsertTask(
      {.type_definition_id = scada::data_items::id::DiscreteItemType,
       .parent_id = scada::data_items::id::DataItems,
       .properties = {{scada::data_items::id::DataItemType_Output,
                       MakeNodeIdFormula(kIec61850DataObject)}}}));

  ASSERT_TRUE(node_id.ok());
  EXPECT_EQ(WrittenTwoStaged(), std::nullopt);
}

// Backlog 1031: since backlog 1015 the insert asks for every property's
// declaration before it creates anything. Measured here against the
// production node service (v3) rather than reasoned about: what a ~1000-row
// Multiple Create costs in declaration round trips. A declaration is fetched
// once and then served from the node cache, because each insert touches it and
// keeps it at the front of the service's most-recently-used window -- even
// while the views read far more new nodes than that window holds. So no
// per-type cache is added in the task manager; this test is what says so.
class CountingNodeFetcher : public v3::NodeFetcher {
 public:
  explicit CountingNodeFetcher(scada::AddressSpace& address_space)
      : address_space_{address_space} {}

  scada::CoStatusOr<scada::NodeState> FetchNode(
      const scada::NodeId& node_id) override {
    ++fetches[node_id];
    const auto* node = address_space_.GetNode(node_id);
    if (!node)
      co_return scada::StatusCode::Bad_WrongNodeId;
    co_return scada::MakeNodeState(*node);
  }

  scada::CoStatusOr<scada::ReferenceDescriptions> FetchChildren(
      const scada::NodeId& node_id) override {
    co_return scada::ReferenceDescriptions{};
  }

  std::map<scada::NodeId, int> fetches;

 private:
  scada::AddressSpace& address_space_;
};

class TaskManagerInsertCostTest : public Test {
 protected:
  void SetUp() override {
    node_service_->OnChannelOpened();
    // The server refuses every insert after the declaration gate: the gate's
    // cost is what is measured, and the refusal ends each task there.
    ON_CALL(node_management_service_, AddNodes(_, _))
        .WillByDefault(
            [](scada::ServiceContext, std::vector<scada::AddNodesItem>) {
              return scada::MakeNodeManagementResult<scada::AddNodesResult>(
                  scada::Status{scada::StatusCode::Bad});
            });
  }

  template <class T>
  T Wait(Awaitable<T> pending) {
    auto result = StartAwaitable(executor_, std::move(pending));
    while (!result->done) {
      executor_.Advance(10ms);
      Drain(executor_);
    }
    executor_.Poll();
    return WaitResult(executor_, std::move(result));
  }

  scada_test::ScadaTestAddressSpace address_space_;
  TestExecutor executor_;
  NiceMock<scada::MockMonitoredItemService> monitored_item_service_;
  std::shared_ptr<CountingNodeFetcher> fetcher_ =
      std::make_shared<CountingNodeFetcher>(address_space_);
  // The production window size: the default of NodeServiceImplContext.
  std::shared_ptr<v3::NodeServiceImpl> node_service_ =
      std::make_shared<v3::NodeServiceImpl>(v3::NodeServiceImplContext{
          .executor_ = executor_,
          .monitored_item_service_ = monitored_item_service_,
          .node_fetcher_ = fetcher_,
          .view_events_provider_ = [](scada::ViewEvents&) {
            return std::make_unique<IViewEventsSubscription>();
          }});
  NiceMock<scada::MockAttributeService> attribute_service_;
  NiceMock<scada::MockNodeManagementService> node_management_service_;
  LocalEvents local_events_;
  Profile profile_;
  ProgressHostImpl progress_host_;
  const std::shared_ptr<TaskManagerImpl> task_manager_ =
      std::make_shared<TaskManagerImpl>(TaskManagerImplContext{
          .executor_ = executor_,
          .node_service_ = *node_service_,
          .attribute_service_ = attribute_service_,
          .node_management_service_ = node_management_service_,
          .local_events_ = local_events_,
          .profile_ = profile_,
          .progress_host_ = progress_host_});
};

TEST_F(TaskManagerInsertCostTest, AThousandInsertsFetchEachDeclarationOnce) {
  constexpr int kRows = 1000;
  // More new nodes per row than the 4096-node window can hold over the run:
  // the open views fetch every row the create adds, and its properties.
  constexpr int kNodesReadPerRow = 8;
  const std::vector<scada::NodeId> declarations{
      scada::data_items::id::DataItemType_Alias,
      scada::data_items::id::DataItemType_Input1,
      scada::data_items::id::DataItemType_Severity};

  // The control: a node read once and never again must be evicted by that
  // churn, or the run below never tested the window at all.
  const scada::NodeId control = scada::data_items::id::AnalogItemType;
  ASSERT_TRUE(Wait(FetchNodeStatus(node_service_->GetNode(control))));

  for (int row = 0; row < kRows; ++row) {
    scada::NodeProperties properties;
    for (const scada::NodeId& declaration : declarations)
      properties.emplace_back(declaration, scada::Variant{});
    (void)Wait(task_manager_->PostInsertTask(
        {.type_definition_id = scada::data_items::id::DiscreteItemType,
         .parent_id = scada::data_items::id::DataItems,
         .properties = std::move(properties)}));

    for (int i = 0; i < kNodesReadPerRow; ++i) {
      node_service_
          ->GetNode(scada::NodeId{static_cast<scada::NumericId>(
                                      1'000'000 + row * kNodesReadPerRow + i),
                                  scada::NamespaceIndexes::TS})
          .StartFetch(NodeFetchStatus::NodeOnly);
    }
    Drain(executor_);
  }

  ASSERT_TRUE(Wait(FetchNodeStatus(node_service_->GetNode(control))));
  EXPECT_EQ(fetcher_->fetches[control], 2);

  for (const scada::NodeId& declaration : declarations)
    EXPECT_EQ(fetcher_->fetches[declaration], 1) << declaration.ToString();
  EXPECT_EQ(fetcher_->fetches[scada::data_items::id::DiscreteItemType], 1);
}
