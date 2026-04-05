#include "configuration/objects/object_tree_view.h"

#include "aui/tree.h"
#include "client_utils.h"
#include "configuration/tree/configuration_tree_drop_handler.h"
#include "configuration/objects/object_tree_model.h"
#include "controller/contents_model.h"
#include "controller/controller_delegate.h"
#include "model/data_items_node_ids.h"
#include "node_service/node_service.h"
#include "profile/profile.h"

ObjectTreeView::ObjectTreeView(const ControllerContext& context)
    : ConfigurationTreeView{context, CreateConfigurationTreeModel(context),
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
    }
  });

  controller_delegate_.AddContentsObserver(*this);

  model().AddObserver(*this);
}

ObjectTreeView::~ObjectTreeView() {
  controller_delegate_.RemoveContentsObserver(*this);

  model().RemoveObserver(*this);
}

// static
std::shared_ptr<ConfigurationTreeModel>
ObjectTreeView::CreateConfigurationTreeModel(const ControllerContext& context) {
  auto model = std::make_shared<ObjectTreeModel>(ObjectTreeModelContext{
      context.executor_,
      context.node_service_,
      context.node_service_.GetNode(data_items::id::DataItems),
      context.timed_data_service_,
      context.profile_,
      context.blinker_manager_,
  });
  model->Init();
  return model;
}

// static
std::unique_ptr<ConfigurationTreeDropHandler>
ObjectTreeView::CreateTreeDropHandler(const ControllerContext& context) {
  return std::make_unique<ConfigurationTreeDropHandler>(
      ConfigurationTreeDropHandlerContext{
          context.node_service_, context.task_manager_, context.create_tree_});
}

ObjectTreeModel& ObjectTreeView::model() {
  return static_cast<ObjectTreeModel&>(ConfigurationTreeView::model());
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

void ObjectTreeView::OnTreeNodesAdded(void* parent, int start, int count) {
  auto& parent_node = *model().AsNode(parent);
  if (tree_view().IsExpanded(&parent_node, true)) {
    for (int i = 0; i < count; ++i) {
      auto& child = parent_node.GetChild(start + i);
      model().SetNodeVisible(&child, true);
    }
  }
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
