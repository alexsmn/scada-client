#include "components/object_tree/object_tree_model.h"

#include "core/standard_node_ids.h"

ObjectTreeModel::ObjectTreeModel(ObjectTreeModelContext&& context)
    : ObjectTreeModelContext{std::move(context)},
      ConfigurationTreeModel{::ConfigurationTreeModelContext{
          ObjectTreeModelContext::node_service_,
          ObjectTreeModelContext::task_manager_,
          ObjectTreeModelContext::root_,
          {{scada::id::Organizes, true}},
          {},
      }},
      visible_node_model_{
          timed_data_service_,
          profile_,
          blinker_manager_,
          [this](void* tree_node) { TreeNodeChanged(tree_node); },
      } {}

int ObjectTreeModel::GetColumnCount() const {
  return 2;
}

std::wstring ObjectTreeModel::GetColumnText(int column_id) const {
  return column_id == 0 ? L"Имя" : L"Значение";
}

int ObjectTreeModel::GetColumnPreferredSize(int column_id) const {
  return column_id == 0 ? 200 : 0;
}

std::wstring ObjectTreeModel::GetText(void* tree_node, int column_id) {
  if (column_id == 1)
    return visible_node_model_.GetText(tree_node);
  else
    return ConfigurationTreeModel::GetText(tree_node, column_id);
}

SkColor ObjectTreeModel::GetTextColor(void* tree_node, int column_id) {
  if (column_id == 1)
    return visible_node_model_.GetTextColor(tree_node);
  else
    return ConfigurationTreeModel::GetTextColor(tree_node, column_id);
}

SkColor ObjectTreeModel::GetBackgroundColor(void* tree_node, int column_id) {
  if (column_id == 1)
    return visible_node_model_.GetBackgroundColor(tree_node);
  else
    return ConfigurationTreeModel::GetBackgroundColor(tree_node, column_id);
}

void ObjectTreeModel::SetNodeVisible(void* tree_node, bool visible) {
  auto& configuration_tree_node =
      *static_cast<ConfigurationTreeNode*>(tree_node);
  if (visible) {
    auto visible_node = std::make_unique<ObjectVisibleNode>(
        timed_data_service_, configuration_tree_node.node());
    visible_node_model_.SetNode(tree_node, std::move(visible_node));
  } else {
    visible_node_model_.SetNode(tree_node, nullptr);
  }
}
