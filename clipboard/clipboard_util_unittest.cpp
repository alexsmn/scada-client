#include "clipboard_util.h"

#ifdef _WIN32
#include "base/win/clipboard.h"
#endif
#include "base/test/awaitable_test.h"
#include "base/test/test_executor.h"
#include "model/data_items_node_ids.h"
#include "model/namespaces.h"
#include "node_service/node_fetch_status.h"
#include "node_service/test/fake_node_service.h"
#include "services/task_manager_mock.h"
#include "services/test/test_task_manager.h"

#ifdef _WIN32
#include <Windows.h>
#endif

#include <gmock/gmock.h>

#include <optional>
#include <thread>

#include "base/debug_util.h"
#include "scada/co_result.h"

using namespace testing;

namespace {

#ifdef _WIN32
const UINT kNodeTreeFormat =
    ::RegisterClipboardFormat(L"EFCAD60E-2623-4eef-8DE9-9B030DCD3AFE");

void ClearClipboard() {
  Clipboard clipboard;
  ASSERT_TRUE(clipboard.Open());
  ASSERT_TRUE(::EmptyClipboard());
}

void SetNodeTreeClipboardData(std::string_view data) {
  Clipboard clipboard;
  ASSERT_TRUE(clipboard.Open());
  ASSERT_TRUE(::EmptyClipboard());
  ASSERT_TRUE(clipboard.SetData(kNodeTreeFormat, data.data(), data.size()));
}
#endif

void CompareRecursive(const scada::NodeState& a, const scada::NodeState& b) {
  EXPECT_EQ(a.type_definition_id, b.type_definition_id);
  EXPECT_EQ(a.attributes, b.attributes);
  EXPECT_EQ(a.properties, b.properties);
  EXPECT_EQ(a.references, b.references);

  ASSERT_EQ(a.children.size(), b.children.size());

  for (size_t i = 0; i < a.children.size(); ++i) {
    CompareRecursive(a.children[i], b.children[i]);
  }
}

}  // namespace

// Regression (backlog 714): CopyNodesToClipboard co_spawned on a fresh
// ThreadExecutor, so FetchNode ran before the first suspension and the
// subsequent BuildNodeTree walk read references, type definitions and
// property values of every selected node -- all from a worker, off the
// unlocked, executor-affine NodeService cache the GUI thread mutates
// (node_service.h). Asserting on the *thread* the fetch lands on is what
// pins it: the old code recorded a different one.
TEST(CopyNodesToClipboard, RunsOnTheCallersExecutorAndNotAPrivateThread) {
  TestExecutor executor;
  FakeNodeService node_service;

  const scada::NodeId node_id{7, 1};
  node_service.Add(scada::NodeState{.node_id = node_id});
  // The fake registers nodes fully fetched, and FetchNode short-circuits on
  // one; unfetched, the copy actually reaches the service.
  node_service.SetFetchStatus(node_id, NodeFetchStatus::None);

  std::optional<std::thread::id> fetch_thread;
  node_service.SetFetchHandler(node_id,
                               [&](const NodeFetchStatus&) -> Awaitable<void> {
                                 fetch_thread = std::this_thread::get_id();
                                 co_return;
                               });

  CopyNodesToClipboard(executor, {node_service.GetNode(node_id)});

  // Nothing has run yet: the work is queued on this executor, not started on
  // a thread of its own.
  EXPECT_FALSE(fetch_thread.has_value());

  for (int i = 0; i < 100 && executor.GetTaskCount() != 0; ++i)
    executor.Poll();

  ASSERT_TRUE(fetch_thread.has_value());
  EXPECT_EQ(*fetch_thread, std::this_thread::get_id());
}

TEST(PasteNodesFromNodeStateRecursive, Test) {
  TestExecutor executor;
  TestStorage storage{scada::data_items::id::DataItems};
  TestTaskManager task_manager{storage};

  // Have to specify parent IDs for children to be able to compare them.
  const auto& top_node = scada::NodeState{
      .node_id = {1, scada::NamespaceIndexes::GROUP},
      .type_definition_id = scada::data_items::id::DataGroupType,
      .parent_id = scada::data_items::id::DataItems,
      .reference_type_id = scada::id::Organizes,
      .attributes = {.browse_name = "Group1", .display_name = u"Group1"},
      .children = {
          {.node_id = {2, scada::NamespaceIndexes::GROUP},
           .type_definition_id = scada::data_items::id::DataGroupType,
           .parent_id = {1, scada::NamespaceIndexes::GROUP},
           .reference_type_id = scada::id::Organizes,
           .attributes = {.browse_name = "Group2", .display_name = u"Group2"},
           .children = {
               {.node_id = {1, scada::NamespaceIndexes::TS},
                .type_definition_id = scada::data_items::id::DiscreteItemType,
                .parent_id = {2, scada::NamespaceIndexes::GROUP},
                .reference_type_id = scada::id::Organizes,
                .attributes = {.browse_name = "DataItem1",
                               .display_name = u"DataItem1"}}}}}};

  WaitAwaitable(executor, PasteNodesFromNodeStateRecursive(
                              task_manager, scada::NodeState{top_node}));

  auto* storage_root_node = storage.FindNode(scada::data_items::id::DataItems);
  ASSERT_THAT(storage_root_node, NotNull());
  ASSERT_THAT(storage_root_node->children, SizeIs(1));

  CompareRecursive(top_node, storage_root_node->children[0]);
}

TEST(PasteNodesFromNodeStateRecursive, RejectedInsertPropagates) {
  TestExecutor executor;
  StrictMock<MockTaskManager> task_manager;

  scada::NodeState node_state{
      .node_id = {1, scada::NamespaceIndexes::GROUP},
      .type_definition_id = scada::data_items::id::DataGroupType,
      .parent_id = scada::data_items::id::DataItems,
      .reference_type_id = scada::id::Organizes,
      .attributes = {.browse_name = "Group1", .display_name = u"Group1"}};

  EXPECT_CALL(task_manager, PostInsertTask(_))
      .WillOnce(Invoke(
          [](const scada::NodeState&) -> scada::CoStatusOr<scada::NodeId> {
            co_return scada::StatusCode::Bad;
          }));

  WaitAwaitable(executor, PasteNodesFromNodeStateRecursive(
                              task_manager, std::move(node_state)));
}

TEST(PasteNodesFromNodeStateRecursive,
     RemovesInverseReferencesAndAssignsChildParent) {
  TestExecutor executor;
  StrictMock<MockTaskManager> task_manager;

  const scada::NodeId inserted_parent_id{10, scada::NamespaceIndexes::GROUP};
  const scada::NodeId inserted_child_id{11, scada::NamespaceIndexes::TS};

  scada::NodeState node_state{
      .node_id = {1, scada::NamespaceIndexes::GROUP},
      .type_definition_id = scada::data_items::id::DataGroupType,
      .parent_id = scada::data_items::id::DataItems,
      .reference_type_id = scada::id::Organizes,
      .attributes = {.browse_name = "Group1", .display_name = u"Group1"},
      .references = {{.reference_type_id = scada::id::HasTypeDefinition,
                      .forward = true,
                      .node_id = scada::data_items::id::DataGroupType},
                     {.reference_type_id = scada::id::Organizes,
                      .forward = false,
                      .node_id = scada::data_items::id::DataItems}},
      .children = {
          {.node_id = {1, scada::NamespaceIndexes::TS},
           .type_definition_id = scada::data_items::id::DiscreteItemType,
           .parent_id = {1, scada::NamespaceIndexes::GROUP},
           .reference_type_id = scada::id::Organizes,
           .attributes = {.browse_name = "DataItem1",
                          .display_name = u"DataItem1"}}}};

  {
    InSequence sequence;

    EXPECT_CALL(task_manager, PostInsertTask(_))
        .WillOnce(Invoke([&](const scada::NodeState& inserted)
                             -> scada::CoStatusOr<scada::NodeId> {
          EXPECT_TRUE(inserted.children.empty());
          EXPECT_THAT(inserted.references, SizeIs(1));
          if (!inserted.references.empty())
            EXPECT_TRUE(inserted.references.front().forward);
          co_return inserted_parent_id;
        }));

    EXPECT_CALL(task_manager, PostInsertTask(_))
        .WillOnce(Invoke([&](const scada::NodeState& inserted)
                             -> scada::CoStatusOr<scada::NodeId> {
          EXPECT_TRUE(inserted.children.empty());
          EXPECT_EQ(inserted.parent_id, inserted_parent_id);
          EXPECT_EQ(inserted.type_definition_id,
                    scada::data_items::id::DiscreteItemType);
          co_return inserted_child_id;
        }));
  }

  EXPECT_NO_THROW(WaitAwaitable(
      executor,
      PasteNodesFromNodeStateRecursive(task_manager, std::move(node_state))));
}

#ifdef _WIN32
TEST(PasteNodesFromClipboard, EmptyClipboardRejects) {
  TestExecutor executor;
  ClearClipboard();
  StrictMock<MockTaskManager> task_manager;

  EXPECT_THROW(WaitAwaitable(executor, PasteNodesFromClipboard(
                                           task_manager,
                                           scada::data_items::id::DataItems)),
               std::runtime_error);
}

TEST(PasteNodesFromClipboard, InvalidClipboardPayloadRejects) {
  TestExecutor executor;
  SetNodeTreeClipboardData("not a serialized node tree");
  StrictMock<MockTaskManager> task_manager;

  EXPECT_THROW(WaitAwaitable(executor, PasteNodesFromClipboard(
                                           task_manager,
                                           scada::data_items::id::DataItems)),
               std::runtime_error);
}
#endif
