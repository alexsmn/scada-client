#include "configuration/objects/object_tree_view.h"

#include "aui/tree.h"
#include "configuration/objects/object_tree_model.h"
#include "configuration/tree/configuration_tree_drop_handler.h"
#include "controller/contents_model.h"
#include "controller/controller_delegate.h"
#include "model/data_items_node_ids.h"
#include "node_service/node_format.h"
#include "node_service/node_service.h"
#include "node_service/node_util.h"
#include "profile/profile.h"
#include "ui/common/client_utils.h"

namespace {

ConfigurationTreeNode* FindFirstValueTreeNode(ConfigurationTreeNode& node) {
  if (IsInstanceOf(node.node(), scada::data_items::id::DataItemType))
    return &node;

  if (node.CanFetchMore())
    node.FetchMore();

  for (int i = 0; i < node.GetChildCount(); ++i) {
    if (auto* value_node = FindFirstValueTreeNode(node.GetChild(i)))
      return value_node;
  }

  return nullptr;
}

}  // namespace

ObjectTreeView::ObjectTreeView(
    const ControllerContext& context,
    const NodeServiceTreeFactory& node_service_tree_factory)
    : ConfigurationTreeView{
          context,
          CreateConfigurationTreeModel(context, node_service_tree_factory),
          CreateTreeDropHandler(context)} {
  tree_view().SetHeaderVisible(true);
  tree_view().SetShowChecks(true);

  tree_view().SetExpandedHandler([this](void* node, bool expanded) {
    UpdateNodesVisibility(*static_cast<ConfigurationTreeNode*>(node), expanded);
  });

  tree_view().SetCheckedHandler([this](void* node, bool checked) {
    ConfigurationTreeNode* n = reinterpret_cast<ConfigurationTreeNode*>(node);
    auto* contents_model = controller_delegate_.GetActiveContentsModel();
    if (contents_model) {
      // Objects must be added in the order as they are listed in the tree view.
      auto node_ids =
          GetVariableNodeIds(tree_view().GetOrderedNodes(n, !checked));
      for (auto& node_id : node_ids) {
        if (checked)
          contents_model->AddContainedItem(node_id, ContentsModel::APPEND);
        else
          contents_model->RemoveContainedItem(node_id);
      }
      // The mark follows the contents, and the contents report only *changes*:
      // adding an item the view already holds is silently a no-op, so a mark
      // that had drifted would otherwise be uncorrectable — clicking it would
      // do nothing at all. Re-derive every mark from what the view holds now.
      SetContents(contents_model->GetContainedItems());
    }
  });

  model_connections_.push_back(controller_delegate_.SubscribeContentsChanged(
      [this](const NodeIdSet& node_ids) { OnContentsChanged(node_ids); }));
  model_connections_.push_back(
      controller_delegate_.SubscribeContainedItemChanged(
          [this](const scada::NodeId& item_id, bool added) {
            OnContainedItemChanged(item_id, added);
          }));

  model_connections_.push_back(model().SubscribeNodeChanged(
      [this](void* node) { OnTreeNodeChanged(node); }));
  model_connections_.push_back(
      model().SubscribeNodesAdded([this](void* parent, int start, int count) {
        OnTreeNodesAdded(parent, start, count);
      }));
  model_connections_.push_back(model().SubscribeNodesDeleting(
      [this](void* parent, int start, int count) {
        OnTreeNodesDeleting(parent, start, count);
      }));
  model_connections_.push_back(
      model().SubscribeModelResetting([this] { OnTreeModelResetting(); }));

  // Catch up on the rows that already exist.
  //
  // The model is built and Init()ed by CreateConfigurationTreeModel, which runs
  // in this constructor's initializer list -- before the subscriptions above.
  // The root's first level is populated there, so its nodes-added notifications
  // fire with nobody listening, and OnTreeNodesAdded never sees the root as a
  // parent. Every top-level row therefore missed its live value subscription
  // permanently: not a timing wobble, a window that closes before the observer
  // is attached. Parity finding V45.
  if (auto* root_node = model().root()) {
    UpdateNodesVisibility(*root_node, true);
  }
}

ObjectTreeView::~ObjectTreeView() = default;

std::optional<std::u16string> ObjectTreeView::GetFirstValueTextForTesting() {
  if (!value_node_for_testing_) {
    auto* root = model().root();
    if (!root)
      return std::nullopt;

    value_node_for_testing_ = FindFirstValueTreeNode(*root);
    if (!value_node_for_testing_)
      return std::nullopt;

    value_node_change_count_for_testing_ = 0;
    model().SetNodeVisible(value_node_for_testing_, true);
  }

  auto value_text = model().GetText(value_node_for_testing_, /*column_id=*/1);
  if (!value_text.empty() && value_node_change_count_for_testing_ != 0)
    return value_text;

  auto node = value_node_for_testing_->node();
  if (!node.fetched())
    return std::nullopt;

  value_text =
      FormatValue(node, node.value(), scada::Qualifier{}, FORMAT_DEFAULT);
  if (value_text.empty())
    return std::nullopt;

  return value_text;
}

std::vector<std::u16string> ObjectTreeView::GetExpandedLabelPathForTesting(
    int levels) {
  auto* node = model().root();
  std::vector<std::u16string> labels;
  if (!node)
    return labels;

  auto expand_path = [&](auto& self, ConfigurationTreeNode& current,
                         int depth) -> bool {
    if (current.CanFetchMore())
      current.FetchMore();

    tree_view().ExpandNode(&current);
    if (&current != model().root())
      model().SetNodeVisible(&current, true);
    labels.emplace_back(model().GetText(&current, /*column_id=*/0));

    if (depth == levels)
      return true;

    for (int i = 0; i < current.GetChildCount(); ++i) {
      const auto previous_size = labels.size();
      if (self(self, current.GetChild(i), depth + 1))
        return true;
      labels.resize(previous_size);
    }

    return false;
  };

  expand_path(expand_path, *node, /*depth=*/0);

  return labels;
}

// static
std::shared_ptr<ConfigurationTreeModel>
ObjectTreeView::CreateConfigurationTreeModel(
    const ControllerContext& context,
    const NodeServiceTreeFactory& node_service_tree_factory) {
  auto model = std::make_shared<ObjectTreeModel>(ObjectTreeModelContext{
      context.executor_,
      context.node_service_,
      context.node_service_.GetNode(scada::data_items::id::DataItems),
      context.timed_data_service_,
      context.profile_,
      context.blinker_manager_,
      node_service_tree_factory,
  });
  model->Init();
  return model;
}

// static
std::unique_ptr<ConfigurationTreeDropHandler>
ObjectTreeView::CreateTreeDropHandler(const ControllerContext& context) {
  return std::make_unique<ConfigurationTreeDropHandler>(
      ConfigurationTreeDropHandlerContext{
          context.executor_, context.node_service_, context.task_manager_,
          context.create_tree_});
}

ObjectTreeModel& ObjectTreeView::model() {
  return static_cast<ObjectTreeModel&>(ConfigurationTreeView::model());
}

bool ObjectTreeView::ShowsChildren(ConfigurationTreeNode& parent_node) {
  // The root's children are the top-level rows, and they are on screen from
  // the moment they exist: the root itself is never drawn (the pane's title
  // names its contents), so the view holds no expansion state for it and
  // `IsExpanded` answers false for it forever. Without the first term a row
  // added under the root after construction would never be subscribed.
  //
  // This is NOT what fixed V45, and the distinction is worth keeping. Tracing
  // the real fixture showed `parent == root()` is never true here, because the
  // root's first level is populated during the model's Init() -- before this
  // view subscribes at all -- so those notifications reach nobody and this
  // function is not called for them. The constructor's catch-up is what
  // repairs that. This term covers the other case: children arriving under the
  // root *later*, while the view is listening.
  //
  // The teardown side already assumed both: OnTreeModelResetting hands the
  // root to UpdateNodesVisibility to unsubscribe exactly these children.
  return &parent_node == model().root() ||
         tree_view().IsExpanded(&parent_node, true);
}

void ObjectTreeView::UpdateNodesVisibility(ConfigurationTreeNode& parent_node,
                                           bool expanded) {
  for (int i = 0; i < parent_node.GetChildCount(); ++i) {
    auto& child = parent_node.GetChild(i);
    model().SetNodeVisible(&child, expanded);
    if (tree_view().IsExpanded(&child, false))
      UpdateNodesVisibility(child, expanded);
  }
}

void ObjectTreeView::OnTreeNodeChanged(void* node) {
  if (node == value_node_for_testing_)
    ++value_node_change_count_for_testing_;
}

void ObjectTreeView::OnTreeNodesAdded(void* parent, int start, int count) {
  auto& parent_node = *model().AsNode(parent);
  if (ShowsChildren(parent_node)) {
    for (int i = 0; i < count; ++i) {
      auto& child = parent_node.GetChild(start + i);
      model().SetNodeVisible(&child, true);
    }
  }

  // Seed the new nodes' marks from the contents. Without this a node never
  // gets one: the tree is materialized lazily, so a page that restored its
  // contents at login did so before any of these nodes existed, and
  // OnContentsChanged — which resolves contents through FindTreeNodes — could
  // only see the nodes that happened to exist at that moment. Nothing else
  // reconciles a node that appears later, so every mark for a restored page
  // stayed clear while the table listed exactly those items.
  //
  // Children arrive one at a time (ConfigurationTreeModel::UpdateChildTreeNodes
  // adds them in a loop), so an ancestor is re-derived here against a partially
  // materialized set. That is correct on convergence — the last child's
  // notification settles it — and no repaint happens inside the loop.
  if (contents_.empty())
    return;

  for (int i = 0; i < count; ++i) {
    auto& child = parent_node.GetChild(start + i);
    tree_view().SetChecked(&child, IsCheckedByContents(&child, contents_));
  }
  SyncCheckedState(parent, contents_);
}

bool ObjectTreeView::IsCheckedByContents(void* node,
                                         const NodeIdSet& contents) {
  auto& tree_node = *static_cast<ConfigurationTreeNode*>(node);
  if (contents.contains(tree_node.node().node_id()))
    return true;

  // A node that holds no value of its own is marked exactly when everything
  // under it is. Children carry settled marks by the time this runs, so it
  // reads them rather than recursing.
  const int child_count = model().GetChildCount(node);
  if (child_count == 0)
    return false;

  for (int i = 0; i < child_count; ++i) {
    if (!tree_view().IsChecked(model().GetChild(node, i)))
      return false;
  }
  return true;
}

void ObjectTreeView::SyncCheckedState(void* node, const NodeIdSet& contents) {
  tree_view().SetChecked(node, IsCheckedByContents(node, contents));
  for (void* parent = model().GetParent(node); parent;
       parent = model().GetParent(parent))
    tree_view().SetChecked(parent, IsCheckedByContents(parent, contents));
}

void ObjectTreeView::OnTreeNodesDeleting(void* parent, int start, int count) {
  auto& parent_node = *model().AsNode(parent);
  if (tree_view().IsExpanded(&parent_node, true)) {
    for (int i = 0; i < count; ++i) {
      auto& child = parent_node.GetChild(start + i);
      model().SetNodeVisible(&child, false);
    }
  }

  for (int i = 0; i < count; ++i) {
    auto& child = parent_node.GetChild(start + i);
    tree_view().SetChecked(&child, false);
  }
}

void ObjectTreeView::OnTreeModelResetting() {
  auto* root_node = model().root();
  if (root_node)
    UpdateNodesVisibility(*root_node, false);
}

void ObjectTreeView::OnContentsChanged(const NodeIdSet& node_ids) {
  SetContents(node_ids);
}

void ObjectTreeView::SetContents(NodeIdSet contents) {
  contents_ = std::move(contents);

  const NodeIdSet& node_ids = contents_;
  std::set<void*> checked_nodes;

  std::vector<void*> pending_nodes;
  pending_nodes.reserve(node_ids.size());
  for (const auto& node_id : node_ids) {
    auto nodes = model().FindTreeNodes(node_id);
    pending_nodes.insert(pending_nodes.end(), nodes.begin(), nodes.end());
  }

  std::unordered_map<void* /*parent*/, int /*checked_child_count*/>
      parent_checks;

  while (!pending_nodes.empty()) {
    // Allow adding nulls for performance and remove it then.
    for (auto* node : pending_nodes)
      ++parent_checks[model().GetParent(node)];
    parent_checks.erase(nullptr);

    checked_nodes.insert(pending_nodes.begin(), pending_nodes.end());
    pending_nodes.clear();

    for (auto [parent, checked_child_count] : parent_checks) {
      int child_count = model().GetChildCount(parent);
      if (child_count == checked_child_count)
        pending_nodes.emplace_back(parent);
    }
    parent_checks.clear();
  }

  tree_view().SetCheckedNodes(std::move(checked_nodes));
}

void ObjectTreeView::OnContainedItemChanged(const scada::NodeId& node_id,
                                            bool added) {
  // Keep what the marks mean in step with the view, so a node materialized
  // later is seeded against the current contents and not a stale snapshot.
  if (added)
    contents_.insert(node_id);
  else
    contents_.erase(node_id);

  auto nodes = model().FindTreeNodes(node_id);
  for (void* node : nodes) {
    while (node && tree_view().IsChecked(node) != added) {
      tree_view().SetChecked(node, added);

      auto* parent = model().GetParent(node);
      if (!parent)
        break;

      if (added) {
        bool all_children_checked = true;
        for (int j = 0; j < model().GetChildCount(parent); ++j) {
          auto* child = model().GetChild(parent, j);
          if (!tree_view().IsChecked(child)) {
            all_children_checked = false;
            break;
          }
        }

        if (!all_children_checked)
          break;
      }

      node = parent;
    }
  }
}
