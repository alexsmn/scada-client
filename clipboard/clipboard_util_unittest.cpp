#include "clipboard_util.h"

#include "model/data_items_node_ids.h"
#include "model/namespaces.h"
#include "services/task_manager_mock.h"

#include <gmock/gmock.h>

#include "base/debug_util-inl.h"

using namespace testing;

namespace {

class TestStorage {
 public:
  explicit TestStorage(scada::NodeId root_node_id)
      : root_node_{.node_id = root_node_id} {}

  scada::NodeId Insert(scada::NodeState&& node_state) {
    assert(node_state.node_id.is_null());

    // TODO: Set namespace based on the node type definition.
    scada::NodeId node_id{next_node_id_++, NamespaceIndexes::GROUP};

    auto* parent_node = FindNode(node_state.parent_id);
    if (!parent_node) {
      throw std::runtime_error{"Unknown parent node ID"};
    }

    node_parent_ids_.try_emplace(node_id, node_state.parent_id);

    node_state.node_id = node_id;
    parent_node->children.emplace_back(std::move(node_state));

    return node_id;
  }

  scada::NodeState* FindNode(const scada::NodeId& node_id) {
    if (node_id == root_node_.node_id) {
      return &root_node_;
    }

    auto i = node_parent_ids_.find(node_id);
    if (i == node_parent_ids_.end()) {
      return nullptr;
    }

    auto* parent_node = FindNode(i->second);
    if (!parent_node) {
      return nullptr;
    }

    auto j = std::ranges::find(parent_node->children, node_id,
                               [](auto& x) { return x.node_id; });
    return j != parent_node->children.end() ? std::to_address(j) : nullptr;
  }

  scada::NodeState root_node_;

  std::unordered_map<scada::NodeId, scada::NodeId /*parent_id*/>
      node_parent_ids_;

  scada::NumericId next_node_id_ = 1000;
};

class TestTaskManager : public TaskManager {
 public:
  explicit TestTaskManager(TestStorage& storage) : storage_{storage} {}

  virtual promise<> PostTask(std::u16string_view description,
                             const TaskLauncher& launcher) override {
    return scada::MakeRejectedStatusPromise(scada::StatusCode::Bad);
  }

  virtual promise<scada::NodeId> PostInsertTask(
      const scada::NodeState& node_state) override {
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

TEST(PasteNodesFromNodeStateRecursive, Test) {
  TestStorage storage{data_items::id::DataItems};
  TestTaskManager task_manager{storage};

  // Have to specify parent IDs for children to be able to compare them.
  const scada::NodeState top_node{
      .node_id = {1, NamespaceIndexes::GROUP},
      .type_definition_id = data_items::id::DataGroupType,
      .parent_id = data_items::id::DataItems,
      .reference_type_id = scada::id::Organizes,
      .attributes = {.browse_name = "Group1", .display_name = u"Group1"},
      .children = {
          {.node_id = {2, NamespaceIndexes::GROUP},
           .type_definition_id = data_items::id::DataGroupType,
           .parent_id = {1, NamespaceIndexes::GROUP},
           .reference_type_id = scada::id::Organizes,
           .attributes = {.browse_name = "Group2", .display_name = u"Group2"},
           .children = {{.node_id = {1, NamespaceIndexes::TS},
                         .type_definition_id = data_items::id::DiscreteItemType,
                         .parent_id = {2, NamespaceIndexes::GROUP},
                         .reference_type_id = scada::id::Organizes,
                         .attributes = {.browse_name = "DataItem1",
                                        .display_name = u"DataItem1"}}}}}};

  PasteNodesFromNodeStateRecursive(task_manager, scada::NodeState(top_node))
      .get();

  auto* storage_root_node = storage.FindNode(data_items::id::DataItems);
  ASSERT_THAT(storage_root_node, NotNull());
  ASSERT_THAT(storage_root_node->children, SizeIs(1));

  CompareRecursive(top_node, storage_root_node->children[0]);
}
